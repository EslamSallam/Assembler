// MIPSAssembler.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <regex>
#include <fstream>

using namespace std;


map <string,unsigned unsigned int > Instructions;
map <string, unsigned int > dataAddresses;
map <string, unsigned int > Addresses;
vector <string> input;
vector <string> dataspace;
vector <unsigned int> binary;
int dataoffset = 0;
int labeloffset = 0;

unsigned int iInstructionRegex(string str);
unsigned int jInstructionRegex(string str);
unsigned int rInstructionRegex(string str);
unsigned int labelRegex(string str, unsigned int c);
string binaryConverter(unsigned int num);
void dataExtrection(string str);
void getData();
void getBinary();
void pass1();
void pass2();

enum registers
{
	$zero = 0,
	$at = 1,
	$v0 = 2, $v1= 3,
	$a0 = 4, $a1= 5, $a2= 6, $a3= 7,
	$t0 = 8, $t1= 9, $t2=10, $t3=11, $t4=12, $t5=13, $t6=14, $t7=15,
	$s0 = 16, $s1= 17, $s2= 18, $s3= 19, $s4= 20, $s5= 21, $s6= 22, $s7= 23,
	$t8 = 24, $t9 = 25,
	$k0 = 26, $k1 = 27,
	$gp = 28,
	$sp= 29,
	$fp= 30,
	$ra= 31
};
map<string, registers> reg;
void fillInstructions() {
	Instructions["add"] = 32;
	Instructions["and"] = 36;
	Instructions["sub"] = 34;
	Instructions["nor"] = 39;
	Instructions["or"] = 37;
	Instructions["slt"] = 42;

	Instructions["addi"] = (8 << 26);
	Instructions["lw"] = (35 << 26);
	Instructions["sw"] = (43 << 26);
	Instructions["beq"] = (4 << 26);
	Instructions["bne"] = (5 << 26);

	Instructions["j"] = (2 << 26);
	

	reg["$zero"] = $zero;
	reg["$at"] = $at;
	reg["$v0"] = $v0;
	reg["$v1"] = $v1;
	reg["$a0"] = $a0;
	reg["$a1"] = $a1;
	reg["$a2"] = $a2;
	reg["$a3"] = $a3;
	reg["$t0"] = $t0;
	reg["$t1"] = $t1;
	reg["$t2"] = $t2;
	reg["$t3"] = $t3;
	reg["$t4"] = $t4;
	reg["$t5"] = $t5;
	reg["$t6"] = $t6;
	reg["$t7"] = $t7;
	reg["$t8"] = $t8;
	reg["$t9"] = $t9;
	reg["$s0"] = $s0;
	reg["$s1"] = $s1;
	reg["$s2"] = $s2;
	reg["$s3"] = $s3;
	reg["$s4"] = $s4;
	reg["$s5"] = $s5;
	reg["$s6"] = $s6;
	reg["$s7"] = $s7;
	reg["$k0"] = $k0;
	reg["$k1"] = $k1;
	reg["$gp"] = $gp;
	reg["$sp"] = $sp;
	reg["$fp"] = $fp;
	reg["$ra"] = $ra;
}

unsigned int rInstructionRegex(string str) 
{
	// R Instruction
	regex inst("([a-z]+) ([$][a-z]*[0-9]|[$][a-z]*), ([$][a-z]*[0-9]|[$][a-z]*), ([$][a-z]+[0-9]|[$][a-z]+)");
	smatch matches;
	unsigned int binNum = 0;
	// Show true and false in output
	

	// Determines if there is a match and match 
	// results are returned in matches
	if (regex_search(str, matches, inst)) {
		matches.ready();
		
		if (matches.empty())
		{
			return binNum;
		}
		// Eliminate the previous match and create
		// a new string to search
		binNum = Instructions[matches.str(1)];
		binNum += (reg[matches.str(2)] << 11);
		binNum += (reg[matches.str(3)] << 21);
		binNum += (reg[matches.str(4)] << 16);
		return binNum;
	}
	return binNum;
}
unsigned int iInstructionRegex(string str)
{
	regex ifirstinst("([a-z]+) {1,10}([$][a-z]+[0-9]*|[$][a-z]+), {1,10}([0-9]+|[a-z]+[0-9]*) {0,10}[(]([$]{1}[a-z]+[0-9]*)[)]");
	smatch matches;
	unsigned unsigned int binNum = 0;
	if (regex_search(str, matches, ifirstinst)) {
		matches.ready();
		
		if (matches.empty())
		{
			return binNum;
		}
		binNum = Instructions[matches.str(1)];
		binNum += (reg[matches.str(4)] << 21);
		binNum += (reg[matches.str(2)] << 16);
		if (Addresses[matches.str(3)])
		{
			binNum += dataAddresses[matches.str(3)];
		}
		else {
			stringstream val(matches.str(3));
			unsigned int num = 0;
			val >> num;
			binNum += num;
		}
		return binNum;
	}
	else {
		regex isecondinst("([a-z]+) {1,10}([$][a-z]*[0-9]|[$][a-z]*), {1,10}([$][a-z]*[0-9]|[$][a-z]*), {1,10}([a-z]+[0-9]*)");
		smatch matches2;
		if (regex_search(str, matches2, isecondinst)) {
			matches2.ready();
			
			if (matches2.empty())
			{
				return binNum;
			}
			binNum = Instructions[matches2.str(1)];
			binNum += (reg[matches2.str(2)] << 21);
			binNum += (reg[matches2.str(3)] << 16);
			///////////////////////////////////////////////////////////////////////////////////////////// label must be handled
			unsigned int jm;
			if (Addresses[matches2.str(4)] < labeloffset) {
				jm = 65536;
				jm -= ((labeloffset - Addresses[matches2.str(4)]) + 1);
				
			}
			else {
				jm = (Addresses[matches2.str(4)] - labeloffset);
				jm--;
			}
			binNum += jm;
			return binNum;
		}
	}
	return binNum;
}

unsigned int jInstructionRegex(string str)
{
	regex jinst("([j]{1}) {1,10}([a-z]+[0-9]*)");
	smatch matches;
	unsigned int binNum = 0;
	if (regex_search(str, matches, jinst)) {
		matches.ready();
		
		if (matches.empty())
		{
			return binNum;
		}
	
		binNum = Instructions[matches.str(1)];
		binNum += Addresses[matches.str(2)];
		return binNum;
	}
	return binNum;
}
unsigned int labelRegex(string str,unsigned int c) {
	regex lab("([a-zA-Z]+[0-9]*){1}:{1} {0,10}(.+)?");
	smatch matches;
	if ( regex_search(str, matches, lab)) {
		matches.ready();
		
		Addresses[matches.str(1)] = labeloffset;
		input.push_back(matches.str(2));
		return 1;
	}
	
	return 0;
}
void dataExtrection(string str) {
	regex dat("([a-z]+[0-9]*){1}:{1} {0,10}(.space|.word) {1,10}([0-9]+)");
	smatch matches;
	if (regex_search(str, matches, dat)) {
		matches.ready();
		stringstream val(matches.str(3));
		unsigned int num = 0;
		val >> num;
		if (matches.str(2) == ".space") {
			dataAddresses[matches.str(1)] = dataoffset;
			string s = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
			for (unsigned int i = 0; i < num; i++) {
				dataspace.push_back(s);
				dataoffset += 4;
			}
		}
		else if (matches.str(2) == ".word") {
			dataAddresses[matches.str(1)] = dataoffset;
			Addresses[matches.str(1)] = num;
			string ss = binaryConverter(num);
			dataspace.push_back(ss);
			dataoffset += 4;
		}
	}
}
void pass1() {
	ifstream inpu("input.txt");
	string line;
	if (inpu.is_open())
	{
		unsigned int counter = 0;
		bool flag1 = 0,flag2 = 0;
		while (getline(inpu, line))
		{
			counter++;
			char delimiter = '#';
			char sp = ' ';
			string token = line.substr(0, line.find(delimiter));
			unsigned int i = 0;
			string mip;
			while (i < token.length()) {
				if (token[i] != ' ') {
					mip = token.substr(i, line.find(delimiter));
					break;
				}
				i++;
			}
			if (mip == ".data") {
				flag1 = 1;
			}
			if (mip == ".text") {
				flag1 = 0;
				flag2 = 1;
			}
			if (mip.empty() || mip == ".data" || mip == ".text") {
				continue;
			}
			if (flag1 && !flag2) {
				dataExtrection(mip);
			}
			else if (!flag1 && flag2){
				unsigned int x = labelRegex(mip, counter);
				if (x == 0) {
					input.push_back(mip);
				}
				labeloffset++;
			}
			
			
		}
		inpu.close();
		
	}
	else {
		cout << "reresfsdfdsaf" << endl;
	}
}

void pass2() {
	labeloffset = 0;
	for (unsigned int i = 0; i < input.size(); i++) {
		if (!rInstructionRegex(input.at(i))) {
			if (!iInstructionRegex(input.at(i))) {
				binary.push_back(jInstructionRegex(input.at(i)));
			}
			else {
				binary.push_back(iInstructionRegex(input.at(i)));
			}
		}
		else {
			binary.push_back(rInstructionRegex(input.at(i)));
		}
		labeloffset++;
		
	}
}
string binaryConverter(unsigned int num) {
	unsigned int bin;
	string s = "";
	unsigned int i = 0;
	while (num > 0)
	{
		i++;
		bin = num % 2;
		if (bin) {
			s += "1";
		}
		else {
			s += "0";
		}
		num /= 2;
	}
	for (unsigned int j = 32-i; j > 0; j--) {
		s += "0";
	}
	reverse(s.begin(), s.end());

	return s;

}
void getData() {
	cout << "#Translation of Data Segment" << endl;
	fstream ofs;
	ofs.open("DataSegment.txt", ios::out | ios::trunc);
	for (unsigned int i = 0; i < dataspace.size(); i++) {
		cout << "MEMORY(" << i << ")" << " := \"" << dataspace[i] << "\";" << endl;
		ofs << "MEMORY(" << i << ")" << " := \"" << dataspace[i] << "\";" << endl;
	}
	ofs.close();
}

void getBinary() {
	cout << "#Translation of Code Segment" << endl;
	fstream ofs;
	ofs.open("CodeSegment.txt", ios::out | ios::trunc);
	for (unsigned int i = 0; i < binary.size(); i++) {
		cout << "MEMORY(" << i << ")" << " := \"" << binaryConverter(binary.at(i)) << "\";" << endl;
		ofs << "MEMORY(" << i << ")" << " := \"" << binaryConverter(binary.at(i)) << "\";" << endl;
	}
	ofs.close();
}

int main()
{
	fillInstructions();
	pass1();
	pass2();
	getData();
	getBinary();
}