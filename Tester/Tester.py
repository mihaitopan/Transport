import os, subprocess, json, shutil, hashlib

# the file to run depends on the building type (Debug, Release, x64, x86)
EXE_FILE = "x64\\Debug\\TransportTester.exe" # EXE_FILE should be modified to the corresponding built
PIPENAME = "\\\\.\\pipe\\transport"
CONFIG_FILE = "Tester\\testbed\\config.json"
CMDFILES_FOLDER = "Tester\\testbed\\cmdFiles"
INOUTFILES_FOLDER = "Tester\\testbed\\inOutFiles"

def run():
    with open(CONFIG_FILE, 'r') as fin:
        myList = json.load(fin)

    for myDict in myList: # for every command file in config file we execute the transport tester and assert the desired result
        cmdFile = os.path.join(CMDFILES_FOLDER, myDict["cmdFile"])

        subprocess.call([EXE_FILE, PIPENAME, cmdFile], stdout=subprocess.PIPE)

        for key in myDict["assertEq"]:
            outFile = os.path.join(INOUTFILES_FOLDER, key)
            verifyFile = os.path.join(INOUTFILES_FOLDER, myDict["assertEq"][key])

            assert open(outFile, 'rb').read() == open(verifyFile, 'rb').read()

            open(outFile, 'w')

run()
print("tests ran successfully")