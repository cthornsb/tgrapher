// tgrapher.cpp
// C. R. Thornsberry
// June 15th, 2015
// Generate a root TGraph from a root TTree.
// SYNTAX: ./tgrapher <filename> <treename> <x_branch> <y_branch> [options]

#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>
#include <vector>

#include "TAxis.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TApplication.h"
#include "TCutG.h"

struct data_gate{
	std::string name;
	double value;
	double *ptr;
	bool use;
	TBranch *data_b;
	
	std::vector<double> low;
	std::vector<double> high;
	
	data_gate() : name(""), ptr(&value), use(false) { 
	}
	
	data_gate(std::string name_) : name(name_), ptr(&value), use(false) { 
	}
	
	bool add(const double &low_, const double &high_){
		if(low_ > high_) return false;
		low.push_back(low_);
		high.push_back(high_);
		return true;
	}
	
	bool is_in_range(){ 
		if(!use) return false;
		std::vector<double>::iterator iter1 = low.begin();
		std::vector<double>::iterator iter2 = high.begin();
		for(; iter1 != low.end() && iter2 != high.end(); iter1++, iter2++){
			if(*ptr >= *iter1 && *ptr <= *iter2) return true;
		}
		return false;
	}
	
	std::string getstr(){
		std::stringstream stream;
		std::vector<double>::iterator iter1 = low.begin();
		std::vector<double>::iterator iter2 = high.begin();
		while(iter1 != low.end() && iter2 != high.end()){
			stream << "[" << *iter1 << ", " << *iter2 << "]";
			if(++iter1 != low.end() && ++iter2 != high.end()) // Add a union symbol.
				stream << " U ";
		}
		return stream.str();
	}
};

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <filename> <treename> <x_branch> <y_branch> [options]\n";
	std::cout << "   Available options:\n";
	std::cout << "    --xerror <name>            | Supply the name of the branch containing the x-axis errors.\n";
	std::cout << "    --yerror <name>            | Supply the name of the branch containing the y-axis errors.\n";
	std::cout << "    --save <filename> <name>   | Save the resulting graph to a root file.\n";
	std::cout << "    --gate <name> <low> <high> | Gate the graph on a branch with the given lower/upper limits.\n";
	std::cout << "    --opt <str>                | Specify the TGraph draw option (default='AP').\n";
	std::cout << "    --cut                      | Draw a TCutG around the data and print entries which are within it.\n";
	std::cout << "    --batch                    | Run in batch mode. i.e. do not open a window for plotting.\n";
}

int main(int argc, char* argv[]){
	if(argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)){
		help(argv[0]);
		return 1;
	}
	else if(argc < 5){
		std::cout << " Error: Invalid number of arguments to " << argv[0] << ". Expected 4, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}

	// Branch name array.
	std::string names[4] = {std::string(argv[3]), std::string(argv[4]), "", ""};

	bool batch_mode = false;
	bool use_xerr = false;
	bool use_yerr = false;
	bool use_tcut = false;
	std::vector<data_gate> gates;
	std::string save_name = "";
	std::string graph_name = "";
	std::string draw_opt = "AP";
	int index = 5;
	while(index < argc){
		if(strcmp(argv[index], "--xerror") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--xerror'!\n";
				help(argv[0]);
				return 1;
			}
			names[2] = std::string(argv[++index]);
			use_xerr = true;
		}
		else if(strcmp(argv[index], "--yerror") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--yerror'!\n";
				help(argv[0]);
				return 1;
			}
			names[3] = std::string(argv[++index]);
			use_yerr = true;
		}
		else if(strcmp(argv[index], "--save") == 0){
			if(index + 2 >= argc){
				std::cout << " Error! Missing required argument to '--save'!\n";
				help(argv[0]);
				return 1;
			}
			save_name = std::string(argv[++index]);
			graph_name = std::string(argv[++index]);
		}
		else if(strcmp(argv[index], "--gate") == 0){
			if(index + 3 >= argc){
				std::cout << " Error! Missing required argument to '--gate'!\n";
				help(argv[0]);
				return 1;
			}
			bool found_gate = false;
			std::string gate_name = std::string(argv[index+1]);
			for(std::vector<data_gate>::iterator iter = gates.begin(); iter != gates.end(); iter++){ // Search for pre-existing gates on this branch.
				if(gate_name == iter->name){
					iter->add(atof(argv[index+2]), atof(argv[index+3]));
					found_gate = true;
				}
			}
			if(!found_gate){ // Found no pre-existing gates.
				data_gate temp_gate(gate_name);
				temp_gate.add(atof(argv[index+2]), atof(argv[index+3]));
				gates.push_back(temp_gate);
			}
			index += 3;
		}
		else if(strcmp(argv[index], "--opt") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--opt'!\n";
				help(argv[0]);
				return 1;
			}
			draw_opt = std::string(argv[++index]);
		}
		else if(strcmp(argv[index], "--cut") == 0){
			use_tcut = true;
		}
		else if(strcmp(argv[index], "--batch") == 0){
			batch_mode = true;
		}
		else{ 
			std::cout << " Error! Unrecognized option '" << argv[index] << "'!\n";
			help(argv[0]);
			return 1;
		}
		index++;
	}

	if(use_tcut && batch_mode)
		use_tcut = false;

	if(names[0] == ""){
		std::cout << " Error! No branch name specified for x-axis!\n";
		return 1;
	}
	else if(names[1] == ""){
		std::cout << " Error! No branch name specified for y-axis\n";
		return 1;
	}

	// Load the input file.
	TFile *file = new TFile(argv[1], "READ");
	if(!file->IsOpen()){
		std::cout << " Error! Failed to load input file.\n"; 
		return 1;
	}
	
	// Load the input tree.
	TTree *tree = (TTree*)file->Get(argv[2]);
	if(!tree){
		std::cout << " Error! Failed to load input tree.\n";
		file->Close();
		return 1;
	}
	//tree->SetMakeClass(1);
	
	double values[4] = {0.0, 0.0, 0.0, 0.0};
	double *valptrs[4] = {&values[0], &values[1],
	                      &values[2], &values[3]};
	TBranch *branches[4];

	bool found_prev_branch;

	// Load the main branches
	for(int i = 0; i < 4; i++){
		if(names[i] == ""){ continue; }
	
		found_prev_branch = false;
	
		// Check if the branch was previously loaded.
		for(int j = 0; j < i; j++){
			if(names[i] == names[j]){ // This branch was already loaded.
				valptrs[i] = valptrs[j];
				branches[i] = branches[j];
				found_prev_branch = true;
				break;
			}
		}
		
		if(!found_prev_branch){ // This is a branch which is not currently in use.
			tree->SetBranchAddress(names[i].c_str(), valptrs[i], &branches[i]);
	
			// Check that the branch was loaded properly.
			if(!branches[i]){
				std::cout << " Warning! Failed to load branch '" << names[i] << "'.\n";
				file->Close();
				return 1;
			}
		}
	}
	
	// Load the gating branches
	for(std::vector<data_gate>::iterator iter = gates.begin(); iter != gates.end(); iter++){
		if(iter->name == names[0]){ // The gate is the same branch as the x-axis.
			iter->ptr = &values[0];
			iter->data_b = branches[0];
		}
		else if(iter->name == names[1]){ // The gate is the same branch as the y-axis.
			iter->ptr = &values[1];
			iter->data_b = branches[1];
		}
		else if(iter->name == names[2]){ // The gate is the same branch as the x-axis errors.
			iter->ptr = &values[2];
			iter->data_b = branches[2];
		}
		else if(iter->name == names[3]){ // The gate is the same branch as the y-axis errors.
			iter->ptr = &values[3];
			iter->data_b = branches[3];
		}
		else{ // The gate is a branch which is not currently in use.
			tree->SetBranchAddress(iter->name.c_str(), &iter->value, &iter->data_b);
			
			// Set the gate pointer.
			iter->ptr = &iter->value;
			
			// Check that the branch was loaded properly.
			if(!iter->data_b){ // Mark this gate as do-not-use.
				std::cout << " Warning! Failed to load gate branch '" << iter->name << "'.\n";
				iter->use = false;
			}
			else{ // Mark this as a good gate.
				iter->use = true;
			}
		}
	}

	// Print some information to the screen.
	std::cout << " Graphing " << names[1] << " vs. " << names[0] << std::endl;
	for(std::vector<data_gate>::iterator iter = gates.begin(); iter != gates.end(); iter++){
		if(!iter->use) continue;
		std::cout << "  For " << iter->name << " in range " << iter->getstr() << std::endl;
	}
	if(use_xerr){ std::cout << "  Using " << names[2] << " as x-axis errors\n"; }
	if(use_yerr){ std::cout << "  Using " << names[3] << " as y-axis errors\n"; }
	
	std::vector<double> xval, yval;
	std::vector<double> xerr, yerr;

	bool valid = true;

	// Process all the entries in the tree.
	unsigned int valid_counts = 0;
	std::cout << "  Processing " << tree->GetEntries() << " entries\n";
	for(unsigned int i = 0; i < tree->GetEntries(); i++){
		tree->GetEntry(i);

		// Check if the pair is within one of the gates (if there are any).
		if(!gates.empty()){ 
			valid = false;
			for(std::vector<data_gate>::iterator iter = gates.begin(); iter != gates.end(); iter++){
				if(iter->is_in_range()){
					valid = true;
					break;
				}
			}
		}
		
		// Store the pair if it is a valid point.
		if(valid){
			xval.push_back(*valptrs[0]);
			yval.push_back(*valptrs[1]);

			if(use_xerr || use_yerr){ 
				xerr.push_back(*valptrs[2]);
				yerr.push_back(*valptrs[3]);
			}
			
			valid_counts++;
		}
	}

	// Check if the pair is within one of the gates (if there are any).
	if(!gates.empty())
		std::cout << " Done! Found " << valid_counts << " valid entries in tree.\n";

	// Initialize the graph.
	TGraphErrors *graph;
	if(!(use_xerr || use_yerr)){ graph = new TGraphErrors(xval.size(), xval.data(), yval.data()); }
	else{ graph = new TGraphErrors(xval.size(), xval.data(), yval.data(), xerr.data(), yerr.data()); }
	
	// Initialize the canvas.
	TApplication* rootapp = NULL;
	TCanvas *can = NULL;
	if(!batch_mode){
		rootapp = new TApplication("rootapp", 0, NULL);
		can = new TCanvas("can");
		can->cd();
	}

	// Set graph attributes and draw to the screen.
	graph->SetTitle((names[1] + " vs. " + names[0]).c_str());
	graph->GetXaxis()->SetTitle(names[0].c_str());
	graph->GetXaxis()->SetTitleOffset(1.2);
	graph->GetYaxis()->SetTitle(names[1].c_str());
	graph->GetYaxis()->SetTitleOffset(1.2);
	graph->SetMarkerColor(4);
	graph->SetMarkerStyle(21);
	
	if(!batch_mode){
		graph->Draw(draw_opt.c_str());
		
		// Let the user make a cut on the graph, if required.
		if(!use_tcut){
			can->WaitPrimitive();
		}
		else{
			double xget, yget;
			std::cout << "Draw the TCutG!\n";
			TCutG *cut = (TCutG*)can->WaitPrimitive("CUTG");
			for(int index = 0; index < graph->GetN(); index++){
				graph->GetPoint(index, xget, yget);
				if(cut->IsInside(xget, yget)){
					std::cout << " " << index << "\t" << xget << "\t" << yget << std::endl;
				}
			}
			delete cut;
		}
	}
	
	// Save the graph to a file, if required.
	if(save_name != ""){
		TFile *graph_output = new TFile(save_name.c_str(), "UPDATE");
		graph_output->cd();
		graph->Write(graph_name.c_str());
		graph_output->Close();
		std::cout << " Wrote graph to file '" << save_name << "'\n";
	}
	
	// Cleanup.
	file->Close();
	graph->Delete();
	
	if(!batch_mode){
		can->Close();
		rootapp->Terminate();
	}
	
	return 0;
}
