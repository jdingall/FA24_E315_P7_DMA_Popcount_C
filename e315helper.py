#!/usr/bin/env python3

import argparse
import json
import os
import shutil
import subprocess
import sys
import re


class Helper():

    def __init__(self, vivado=None):

        self.MY_DIR = os.path.dirname(os.path.realpath(__file__))
        self.JF = self.MY_DIR + '/.data.json'
        self.J = self.load_json()

        self.priv_key = self.MY_DIR + '/.id_rsa.xilinx.priv'
        self.vivado = vivado

        self.version = "1.0.0"
    
    def getVersion(self):
        return self.version

    def load_json(self):
        if os.path.exists( self.JF):
            with open(self.JF, 'r') as f:
                return json.load(f)
        else:
            return {"IP": "192.168.2.99", 
                    "Proj": "P4_Popcount", 
                    "fpga_design": "bd_fpga"}

    def save_json(self):
         with open(self.JF, 'w') as f:
            json.dump( self.J, f) 

     

    def run_command(self, command):
        print ('running: ', command)
        result = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
        return result.communicate()


    def build_vivado(self,):
        #sanity check
        if os.path.exists( self.MY_DIR + '/vivado_project/vivado_project.xpr'):
            print ("Found Vivado Project, Skipping.")
            return

        command = 'vivado -mode batch -source ' + self.J['Proj'] + '.tcl'

        if self.vivado != None:
            print ("vivado specified from command line")
            command = command.replace('vivado', self.vivado)

        elif shutil.which('vivado') != None:
            print ("Found Vivado")
        else:
            raise Exception("Vivado not found!")

        print ("Building Vivado Project")
        self.run_command(command) 

    def impl_vivado(self, num_cores = 1):
        #sanity check
        if not os.path.exists( self.MY_DIR + '/vivado_project/vivado_project.xpr'):
            raise Exception ("Project Missing!")

        command = 'vivado -mode batch ' + \
                '-source ' + self.MY_DIR + '/tcl/impl.tcl' + \
                ' -tclargs ' + self.MY_DIR + ' ' + str(num_cores)

        if self.vivado != None:
            print ("vivado specified from command line")
            command = command.replace('vivado', self.vivado)
        elif shutil.which('vivado') != None:
            print ("Found Vivado")
        else:
            raise Exception("Vivado not found!")

        print ("Running Vivado Implimentation")
        self.run_command(command) 



    def init_pynq(self):
        proj = self.J['Proj']
        ssh = 'ssh -i ' + self.priv_key + ' xilinx@'+self.J['IP'] 

        self.run_command('chmod 0600 ' + self.priv_key)
        
        #sanity check
        if self.run_command(ssh + ' "ls ~/jupyter_notebooks/' + proj + '"')[0] != b'':
            print ("Found Pynq project, Skipping")
            return

        commands = [
                    ssh + ' "mkdir -p ~/tmp" ',
                    ssh + ' "cd ~/tmp && git init --bare" ',
                    'git remote remove tmp', 
                    'git remote add tmp xilinx@' + self.J['IP'] + ':~/tmp/',
                    'GIT_SSH_COMMAND=\'ssh -i '+self.priv_key + '\' git push tmp master', 
                    ssh + ' "git clone tmp ~/jupyter_notebooks/' + proj + ' "',
                    ssh + ' "rm -rf ~/tmp" ',
                    'git remote remove pynq', 
                    'git remote add pynq xilinx@' + self.J['IP'] + ':~/jupyter_notebooks/' + proj,
                    'GIT_SSH_COMMAND=\'ssh -i '+self.priv_key + '\' git push pynq master', 
                   ]   
        for command in commands:                     
            self.run_command(command)

    def load_bitstream(self):
        bit =  self.MY_DIR + '/vivado_project/vivado_project.runs/impl_1/' + self.J['fpga_design'] + '_wrapper.bit'

        hwh = self.MY_DIR + '/vivado_project/vivado_project.gen/sources_1/bd/'+self.J['fpga_design']+'/hw_handoff/' + \
                            self.J['fpga_design']+'.hwh'

        if not os.path.exists( hwh):
            print ('trying alternate hwh file path')
            hwh = self.MY_DIR + '/src/vsrc/'+ self.J['fpga_design'] + '/hw_handoff/' + self.J['fpga_design'] + '.hwh'

        scp = 'scp -i ' + self.priv_key
        pynq = 'xilinx@'+self.J['IP'] + ':~/jupyter_notebooks/' + self.J['Proj'] + '/Pynq/'
        commands = [ 
                'chmod 0600 ' + self.priv_key,
                scp + ' ' + bit + ' ' + pynq + 'bitstream.bit', 
                scp + ' ' + hwh + ' ' + pynq + 'bitstream.hwh', 
                   ]
        for command in commands:                     
            self.run_command(command)

    def set_ip(self, IP):
        # https://www.geeksforgeeks.org/python-program-to-validate-an-ip-address/
        regex = '''^(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.( 
            25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$'''
        if not re.search(regex, IP):  
            print("Invalid IP address, not updating")
            return
        
        if IP == self.J['IP']:
            print ("IPs Match, Skipping")
            return 

        print ("Updating IP Address")
        self.J['IP'] = IP
        self.save_json()

        commands = [ 'git remote remove pynq', 
                     'git remote add pynq xilinx@' + self.J['IP'] + ':~/jupyter_notebooks/' + self.J['Proj'],
                   ]   
        for command in commands:                     
            self.run_command(command)


            
class Parser():

    def __init__ (self):
        ap = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)

        sap = ap.add_subparsers(title='command', dest="command")

        init_ap = sap.add_parser('init', help='initialize the project')
        init_ap.set_defaults(function=self.init)
        init_ap.add_argument('--ip', type=str, nargs='?', help="Ip Address of Pynq")
        init_ap.add_argument('--vivadoOnly', action='store_true', 
            help="Initialize only the Vivado project")
        init_ap.add_argument('--pynqOnly', action='store_true', 
            help="Initialize only the Pynq project")
        init_ap.add_argument('--vivado_bin', type=str, nargs='?', 
            help="Path to Vivado binary")


        impl_ap= sap.add_parser('impl', help='syntheize the bitstream')
        impl_ap.add_argument('--maxCores', type=int, nargs='?', default=1,
            help="Number of CPU cores for synthesis")
        impl_ap.add_argument('--vivado_bin', type=str, nargs='?', 
            help="Path to Vivado binary")

        bitstream_ap= sap.add_parser('bitstream', help='write the bitstream to the Pynq')
        bitstream_ap.add_argument('--ip', type=str, nargs='?', help="Ip Address of Pynq")

        setIp_ap= sap.add_parser('setIp', help='set the Pynq\'s IP address')
        setIp_ap.add_argument('--ip', type=str, nargs='?', help="Ip Address of Pynq")

        version_ap = sap.add_parser('version', help='print current version number')

        init_ap.set_defaults(function=self.init)
        args = ap.parse_args()
        
        if not hasattr(self, args.command):
            print ('Unrecognized Command')
            ap.print_help()
            exit(1)
        
        #jump to the correct command function
        getattr(self, args.command)(args)


    def init(self, args):
        print ('Running init')
        if args.vivado_bin:
            h = Helper( vivado=args.vivado_bin)
        else:
            h = Helper()

        if args.ip:
            h.set_ip(args.ip)
        
        if args.vivadoOnly:
            h.build_vivado()
        elif args.pynqOnly:
            h.init_pynq()
        else:
            h.build_vivado()
            h.init_pynq()

    def impl(self, args):
        print ('Running impl')

        if args.vivado_bin:
            h = Helper( vivado=args.vivado_bin)
        else:
            h = Helper()
        h.build_vivado()
        h.impl_vivado( args.maxCores)

    def bitstream(self, args):
        print ('Running bitstream')
        h = Helper()
        if args.ip:
            h.set_ip(args.ip)
        h.load_bitstream()


    def setIp(self, args):
        print ('Running setIp')
        h = Helper()
        
        if args.ip:
            newip = args.ip
        else:
            print ("Please specify a new IP address: ")
            newip = input()
        h.set_ip(newip)

    def version(self,args):
        h = Helper()
        print ('Running E315Helper Version #' + str(h.getVersion()) )

if __name__ == "__main__":

    Parser()


