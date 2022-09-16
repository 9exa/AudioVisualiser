import os
import argparse


FILENAMEIND = 0 #index of input tuple that declares which file to read
VARNAMEIND = 1
TYPEIND = 2 #index of input tuple that declares how to enterperate content of file
RAWDATAIND = 3

TYPEHEX = 0

CHARSPERLINE = 16

def makeArrayString(data):
    linesRaw = [data[i * CHARSPERLINE: (i+1) * CHARSPERLINE] for i in range(len(data) // CHARSPERLINE)]
    linesRaw += [data[len(data) // CHARSPERLINE * CHARSPERLINE:]]
    return "{\n" + ",\n".join(["\t"+", ".join(map(str, line)) for line in linesRaw]) + "\n}"

# breaks a c file into a list of compile tokens
def getFileTokens(cTextFile):
    tokens = []
    currToken = ""
    i = 0
    while i < len(cTextFile):
        #compiler include or logic search for next newLine
        if cTextFile[i] == "#":
            assert len(currToken) == 0
            tokenEnd = cTextFile.find("\n", i)
            if tokenEnd == -1: 
                #EOF
                tokens.append(cTextFile[i:])
                break
            tokens.append(cTextFile[i: tokenEnd])
            i = tokenEnd
        elif i < len(cTextFile) - 1 and cTextFile[i:i+2] == "//":
            #commentLine
            tokenEnd = cTextFile.find("\n", i)
            if tokenEnd == -1: 
                #EOF
                tokens.append(cTextFile[i:])
                break
            tokens.append(cTextFile[i: tokenEnd])
            i = tokenEnd

        elif cTextFile[i] == ";":
            tokens.append(currToken + ";")
            currToken = ""
        elif cTextFile[i] != "\n":
           currToken += cTextFile[i]
        
        i += 1
    return tokens

# Replace the text after the specified variable in the file with a generated string representing file text
# If a variable is not declared in the document it will not be assigned
def injectArrayStrings(outFileText, inputs):
    #go line by line and search for the a specified variable name to set
    outLines = getFileTokens(outFileText)
    for i in range(len(outLines)):
        line = outLines[i]
        for input in inputs:
            varString = f"const char {input[VARNAMEIND]}[]"
            if line.startswith(varString):
                #write that variable equal to input string as an array of unsigned chars
                outLines[i] = line[:len(varString)] + " = " + makeArrayString(input[RAWDATAIND]) + ";"
                break
    #produce the edited outFile Text
    return "\n\n".join(outLines)

parser = argparse.ArgumentParser(description='Writes content of (binary) text files to const char arrays of a cpp file')
parser.add_argument("-o", "--outfile", dest="outFilePath", type=str, default = "embedded.h")
parser.add_argument("inFileArgs", nargs="+")

if __name__ == "__main__":
    parsed = parser.parse_args()

    inputs = []
    for i in range(len(parsed.inFileArgs) // 2):
        #read in files to encode into variables of output file. For now assume they are hex / char / bytecode
        inputs.append([parsed.inFileArgs[2*i], parsed.inFileArgs[2*i+1], TYPEHEX])
    for input in inputs:
        readMode = {
            TYPEHEX : "rb"
        }[input[TYPEIND]]
        with open(input[FILENAMEIND], readMode) as inputFile:
            inputFile.seek(0)
            input.append(inputFile.read())
    print(parsed.outFilePath)
    with open(parsed.outFilePath, "r+") as outFile:
        outFileText = outFile.read()
        newOutFileText = injectArrayStrings(outFileText, inputs)
        outFile.seek(0)
        outFile.write(newOutFileText)