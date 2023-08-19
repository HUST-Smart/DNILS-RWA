#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>

#include <cstring>

#include "Simulator.h"
#include "ThreadPool.h"


using namespace std;

namespace szx {

	// EXTEND[szx][5]: read it from InstanceList.txt.
	static const List<pair<string, int>> instList({
		/*{"ATT",20},
		{"ATT2",113},
		{"brasil",48},
		{"EON",22},
		{"Finland",46},
		{"NSF.1",22},
		{"NSF.3",22},
		{"NSF.12",38},
		{"NSF.48",41},
		{"NSF2.1",21},
		{"NSF2.3",21},
		{"NSF2.12",35},
		{"NSF2.48",39},*/
		/*{ "Y.3.40_seed=4",50 },
		{ "Y.4.60_seed=5",49 },
		{ "Y.5.80_seed=2",59 },
		{ "Z.5x20.80",205 },*/
		//{ "Y.3.100_seed=1",131 },
		/*{ "z.4x25.100",315 },
		{ "z.5x20.100",252 },
		{ "z.6x17.100",217 },
		{ "z.8x13.100",169 },
		{ "z.10x10.100",134 },*/
		/*{ "Z.4x25.20",66 },
		{ "z.6x17.100",217 },
		{ "z.5x20.100",252 },
		{ "z.4x25.100",315 },
	
		{ "z.8x13.100",169 },
		{ "z.10x10.100",134 },
		{ "Y.4.100_seed=5",87 },
		{ "z.10x10.100",134 },
		{ "Z.8x13.100",168 },
		{ "Y.3.100_seed=1",141 },
		{ "Z.5x20.80",205 },
		{ "Y.5.100_seed=1",55 },
		{ "Y.4.100_seed=1",76 },*/
		//{ "Z.4x25.20",66 },
		//{ "Y.3.60_seed=4",78 },
		////{ "Y.5.80_seed=1",45 },
		//{ "Y.4.100_seed=5",80 },
		//{ "z.8x13.100",169 },
		//{ "z.10x10.20",28 },
		//{ "z.10x10.40",54 },
		//{ "z.10x10.60",82 },
		//{ "z.10x10.80",109 },
		//{ "z.10x10.100",134 },
		//{ "Y.3.60_seed=4",78 },
		/*{ "Y.3.80_seed=5",107 },
		{ "Y.5.80_seed=1",45 },
		{ "Y.5.100_seed=1",56 },*/
		//{ "Y.3.60_seed=4",78 },
		/*{ "Y.5.80_seed=1",45 },
		{ "Y.5.100_seed=1",56 },*/
		/*{ "Z.4x25.20",66 },
		{ "Y.3.60_seed=4",78 },
		{ "Y.3.80_seed=5",107 },
		{ "Y.3.100_seed=1",141 },
		{ "Y.5.80_seed=1",45 },
		{ "Y.5.100_seed=1",56 },*/

		/*{ "Z.4x25.20",66 },
		{ "z.6x17.20",44 },
		{ "Y.3.20_seed=5",28 },
		{ "Y.3.60_seed=4",78 },
		{ "Y.3.80_seed=1",113 },
		{ "Y.3.80_seed=3",118 },
		{ "Y.3.80_seed=5",107 },
		{ "Y.3.100_seed=1",141 },
		{ "Y.4.40_seed=5",36 },
		{ "Y.4.80_seed=1",68 },
		{ "Y.4.80_seed=5",70 },
		{ "Y.4.100_seed=1",84 },
		{ "Y.4.100_seed=5",87 },
		{ "Y.5.80_seed=1",45 },
		{ "Y.5.100_seed=1",56 },
		{ "z.8x13.20",33 },
		{ "z.8x13.100",169 },
		{ "z.10x10.20",28 },
		{ "z.10x10.40",54 },*/

		//{"Y.4.100_seed=5",87},

		{"Y.3.20_seed=1",27},
		{"Y.3.20_seed=2",33},
		{"Y.3.20_seed=3",29},
		{"Y.3.20_seed=4",26},
		{"Y.3.20_seed=5",28},
		{"Y.3.40_seed=1",53},
		{"Y.3.40_seed=2",59},
		{"Y.3.40_seed=3",61},
		{"Y.3.40_seed=4",50},
		{"Y.3.40_seed=5",53},
		{"Y.3.60_seed=1",81},
		{"Y.3.60_seed=2",89},
		{"Y.3.60_seed=3",91},
		{"Y.3.60_seed=4",78},
		{"Y.3.60_seed=5",77},
		{"Y.3.80_seed=1",106},
		{"Y.3.80_seed=2",117},
		{"Y.3.80_seed=3",118},
		{"Y.3.80_seed=4",105},
		{"Y.3.80_seed=5",104},
		{"Y.3.100_seed=1",131},
		{"Y.3.100_seed=2",146},
		{"Y.3.100_seed=3",146},
		{"Y.3.100_seed=4",131},
		{"Y.3.100_seed=5",129},
		{"Y.4.20_seed=1",17},
		{"Y.4.20_seed=2",28},
		{"Y.4.20_seed=3",23},
		{"Y.4.20_seed=4",19},
		{"Y.4.20_seed=5",17},
		{"Y.4.40_seed=1",31},
		{"Y.4.40_seed=2",57},
		{"Y.4.40_seed=3",43},
		{"Y.4.40_seed=4",38},
		{"Y.4.40_seed=5",33},
		{"Y.4.60_seed=1",47},
		{"Y.4.60_seed=2",86},
		{"Y.4.60_seed=3",64},
		{"Y.4.60_seed=4",58},
		{"Y.4.60_seed=5",49},
		{"Y.4.80_seed=1",62},
		{"Y.4.80_seed=2",118},
		{"Y.4.80_seed=3",81},
		{"Y.4.80_seed=4",78},
		{"Y.4.80_seed=5",65},
		{"Y.4.100_seed=1",76},
		{"Y.4.100_seed=2",146},
		{"Y.4.100_seed=3",98},
		{"Y.4.100_seed=4",98},
		{"Y.4.100_seed=5",80},
		{"Y.5.20_seed=1",13},
		{"Y.5.20_seed=2",17},
		{"Y.5.20_seed=3",12},
		{"Y.5.20_seed=4",17},
		{"Y.5.20_seed=5",15},
		{"Y.5.40_seed=1",24},
		{"Y.5.40_seed=2",31},
		{"Y.5.40_seed=3",22},
		{"Y.5.40_seed=4",33},
		{"Y.5.40_seed=5",28},
		{"Y.5.60_seed=1",33},
		{"Y.5.60_seed=2",45},
		{"Y.5.60_seed=3",34},
		{"Y.5.60_seed=4",48},
		{"Y.5.60_seed=5",40},
		{"Y.5.80_seed=1",43},
		{"Y.5.80_seed=2",59},
		{"Y.5.80_seed=3",43},
		{"Y.5.80_seed=4",63},
		{"Y.5.80_seed=5",53},
		{"Y.5.100_seed=1",55},
		{"Y.5.100_seed=2",73},
		{"Y.5.100_seed=3",53},
		{"Y.5.100_seed=4",77},
		{"Y.5.100_seed=5",66},
		/*{"Z.4x25.20",66},
		{"Z.4x25.40",126},
		{"Z.4x25.60",192},
		{"Z.4x25.80",257},
		{"Z.4x25.100",312},
		{"Z.5x20.20",54},
		{"Z.5x20.40",101},
		{"Z.5x20.60",154},
		{"Z.5x20.80",205},
		{"Z.5x20.100",250},
		{"Z.6x17.20",44},
		{"Z.6x17.40",84},
		{"Z.6x17.60",128},
		{"Z.6x17.80",171},
		{"Z.6x17.100",216},
		{"Z.8x13.20",33},
		{"Z.8x13.40",63},
		{"Z.8x13.60",96},
		{"Z.8x13.80",129},
		{"Z.8x13.100",168},
		{ "Z.10x10.20",27 },
		{ "Z.10x10.40",51 },
		{ "Z.10x10.60",77 },
		{ "Z.10x10.80",103 },
		{ "Z.10x10.100",125 },*/
		});
		static const List<List<int>> parameter({
			{850,78,5,9},
			//{850,80,4,11},
			//{850,73,3,7},

			/*{850,70,5,5},
			{850,70,5,18},
			{850,71,3,10},
			{850,71,3,3},
			{850,71,4,6},
			{850,71,5,6},
			{850,73,3,10},
			{850,73,3,7},
			{850,73,6,6},
			{850,74,4,3},
			{850,74,4,16},
			{850,74,5,4},
			{850,74,5,6},
			{850,74,6,6},
			{850,74,5,19},
			{850,76,5,9},
			{850,76,3,15},
			{850,76,5,15},
			{850,76,5,17},
			{850,76,6,8},
			{850,78,3,5},
			{850,78,5,5},
			{850,78,4,3},
			{850,78,4,19},
			{850,78,5,9},
			{850,78,4,17},
			{850,78,4,18},
			{850,80,4,11},
			{850,80,5,8},
			{850,80,6,5},
			{850,80,5,17}*/
			});
	static const List < pair<string, int>> startWave({
		/*{ "Y.3.40_seed=4",54 },
		{ "Y.4.60_seed=5",54 },
		{ "Y.5.80_seed=2",59 },
		{ "Z.5x20.80",205 },*/
		//{ "Y.3.100_seed=1",141 },
		/*{ "z.4x25.100",315 },
		{ "z.5x20.100",252 },
		{ "z.6x17.100",217 },
		{ "z.8x13.100",169 },
		{ "z.10x10.100",134 },*/
		/*{ "Z.4x25.20",66 },
		{ "z.6x17.100",217 },
		{ "z.5x20.100",252 },
		{ "z.4x25.100",315 },
		
		
		{ "z.8x13.100",169 },
		{ "z.10x10.100",134 },
		{ "Y.4.100_seed=5",87 },
		{ "z.10x10.100",134 },
		{ "z.8x13.100",169 },
		{ "Y.3.100_seed=1",141 },
		{ "z.5x20.80",205 },
		{ "Y.5.100_seed=1",56 },
		{ "Y.4.100_seed=1",84 },*/
		/*{ "Y.4.100_seed=5",87 },
		{ "z.8x13.100",169 },
		{ "z.10x10.20",28 },
		{ "z.10x10.40",54 },
		{ "z.10x10.60",82 },
		{ "z.10x10.80",109 },
		{ "z.10x10.100",134 },*/

		/*{ "Z.4x25.20",66 },
		{ "Y.3.60_seed=4",78 },
		{ "Y.3.80_seed=5",107 },
		{ "Y.3.100_seed=1",141 },
		{ "Y.5.80_seed=1",45 },
		{ "Y.5.100_seed=1",56 },*/

		/*{ "Z.4x25.20",66 },
		{ "z.6x17.20",44 },
		{ "Y.3.20_seed=5",28 },
		{ "Y.3.60_seed=4",78 },
		{ "Y.3.80_seed=1",113 },
		{ "Y.3.80_seed=3",118 },
		{ "Y.3.80_seed=5",107 },
		{ "Y.3.100_seed=1",141 },
		{ "Y.4.40_seed=5",36 },
		{ "Y.4.80_seed=1",68 },
		{ "Y.4.80_seed=5",70 },
		{ "Y.4.100_seed=1",84 },
		{ "Y.4.100_seed=5",87 },
		{ "Y.5.80_seed=1",45 },
		{ "Y.5.100_seed=1",56 },
		{ "z.8x13.20",33 },
		{ "z.8x13.100",169 },
		{ "z.10x10.20",28 },
		{ "z.10x10.40",54 },*/
		//{"Y.4.100_seed=5",87},

		{"Y.3.20_seed=1",29},
		{"Y.3.20_seed=2",33},
		{"Y.3.20_seed=3",29},
		{"Y.3.20_seed=4",28},
		{"Y.3.20_seed=5",28},
		{"Y.3.40_seed=1",57},
		{"Y.3.40_seed=2",59},
		{"Y.3.40_seed=3",61},
		{"Y.3.40_seed=4",54},
		{"Y.3.40_seed=5",56},
		{"Y.3.60_seed=1",86},
		{"Y.3.60_seed=2",89},
		{"Y.3.60_seed=3",91},
		{"Y.3.60_seed=4",79},
		{"Y.3.60_seed=5",81},
		{"Y.3.80_seed=1",114},
		{"Y.3.80_seed=2",117},
		{"Y.3.80_seed=3",118},
		{"Y.3.80_seed=4",105},
		{"Y.3.80_seed=5",108},
		{"Y.3.100_seed=1",141},
		{"Y.3.100_seed=2",146},
		{"Y.3.100_seed=3",146},
		{"Y.3.100_seed=4",131},
		{"Y.3.100_seed=5",135},
		{"Y.4.20_seed=1",19},
		{"Y.4.20_seed=2",28},
		{"Y.4.20_seed=3",23},
		{"Y.4.20_seed=4",19},
		{"Y.4.20_seed=5",19},
		{"Y.4.40_seed=1",35},
		{"Y.4.40_seed=2",57},
		{"Y.4.40_seed=3",43},
		{"Y.4.40_seed=4",38},
		{"Y.4.40_seed=5",37},
		{"Y.4.60_seed=1",52},
		{"Y.4.60_seed=2",86},
		{"Y.4.60_seed=3",64},
		{"Y.4.60_seed=4",58},
		{"Y.4.60_seed=5",54},
		{"Y.4.80_seed=1",69},
		{"Y.4.80_seed=2",118},
		{"Y.4.80_seed=3",81},
		{"Y.4.80_seed=4",78},
		{"Y.4.80_seed=5",71},
		{"Y.4.100_seed=1",85},
		{"Y.4.100_seed=2",146},
		{"Y.4.100_seed=3",98},
		{"Y.4.100_seed=4",98},
		{"Y.4.100_seed=5",87},
		{"Y.5.20_seed=1",13},
		{"Y.5.20_seed=2",17},
		{"Y.5.20_seed=3",12},
		{"Y.5.20_seed=4",17},
		{"Y.5.20_seed=5",15},
		{"Y.5.40_seed=1",24},
		{"Y.5.40_seed=2",31},
		{"Y.5.40_seed=3",23},
		{"Y.5.40_seed=4",33},
		{"Y.5.40_seed=5",28},
		{"Y.5.60_seed=1",35},
		{"Y.5.60_seed=2",45},
		{"Y.5.60_seed=3",34},
		{"Y.5.60_seed=4",48},
		{"Y.5.60_seed=5",40},
		{"Y.5.80_seed=1",46},
		{"Y.5.80_seed=2",59},
		{"Y.5.80_seed=3",44},
		{"Y.5.80_seed=4",63},
		{"Y.5.80_seed=5",53},
		{"Y.5.100_seed=1",57},
		{"Y.5.100_seed=2",73},
		{"Y.5.100_seed=3",54},
		{"Y.5.100_seed=4",77},
		{"Y.5.100_seed=5",66},
		/*{"Z.4x25.20",66},
		{"Z.4x25.40",127},
		{"Z.4x25.60",193},
		{"Z.4x25.80",258},
		{"z.4x25.100",315},
		{"z.5x20.20",54},
		{"z.5x20.40",101},
		{"z.5x20.60",154},
		{"z.5x20.80",205},
		{"z.5x20.100",252},
		{"z.6x17.20",44},
		{"z.6x17.40",85},
		{"z.6x17.60",129},
		{"z.6x17.80",171},
		{"z.6x17.100",217},
		{"z.8x13.20",33},
		{"z.8x13.40",64},
		{"z.8x13.60",97},
		{"z.8x13.80",130},
		{"z.8x13.100",169},
		{"z.10x10.20",28},
		{"z.10x10.40",54},
		{"z.10x10.60",82},
		{"z.10x10.80",109},
		{"z.10x10.100",134},*/
		/*{"Y.3.20_seed=1",29},
		{"Y.3.20_seed=2",33},
		{"Y.3.20_seed=3",29},
		{"Y.3.20_seed=4",28},
		{"Y.3.20_seed=5",28},
		{"Y.3.40_seed=1",56},
		{"Y.3.40_seed=2",59},
		{"Y.3.40_seed=3",61},
		{"Y.3.40_seed=4",53},
		{"Y.3.40_seed=5",55},
		{"Y.3.60_seed=1",85},
		{"Y.3.60_seed=2",89},
		{"Y.3.60_seed=3",91},
		{"Y.3.60_seed=4",78},
		{"Y.3.60_seed=5",81},
		{"Y.3.80_seed=1",113},
		{"Y.3.80_seed=2",117},
		{"Y.3.80_seed=3",118},
		{"Y.3.80_seed=4",105},
		{"Y.3.80_seed=5",107},
		{"Y.3.100_seed=1",140},
		{"Y.3.100_seed=2",146},
		{"Y.3.100_seed=3",146},
		{"Y.3.100_seed=4",131},
		{"Y.3.100_seed=5",134},
		{"Y.4.20_seed=1",18},
		{"Y.4.20_seed=2",28},
		{"Y.4.20_seed=3",23},
		{"Y.4.20_seed=4",19},
		{"Y.4.20_seed=5",19},
		{"Y.4.40_seed=1",35},
		{"Y.4.40_seed=2",57},
		{"Y.4.40_seed=3",43},
		{"Y.4.40_seed=4",38},
		{"Y.4.40_seed=5",36},
		{"Y.4.60_seed=1",52},
		{"Y.4.60_seed=2",86},
		{"Y.4.60_seed=3",64},
		{"Y.4.60_seed=4",58},
		{"Y.4.60_seed=5",54},
		{"Y.4.80_seed=1",68},
		{"Y.4.80_seed=2",118},
		{"Y.4.80_seed=3",81},
		{"Y.4.80_seed=4",78},
		{"Y.4.80_seed=5",70},
		{"Y.4.100_seed=1",84},
		{"Y.4.100_seed=2",146},
		{"Y.4.100_seed=3",98},
		{"Y.4.100_seed=4",98},
		{"Y.4.100_seed=5",87},
		{"Y.5.20_seed=1",13},
		{"Y.5.20_seed=2",17},
		{"Y.5.20_seed=3",12},
		{"Y.5.20_seed=4",17},
		{"Y.5.20_seed=5",15},
		{"Y.5.40_seed=1",24},
		{"Y.5.40_seed=2",31},
		{"Y.5.40_seed=3",23},
		{"Y.5.40_seed=4",33},
		{"Y.5.40_seed=5",28},
		{"Y.5.60_seed=1",35},
		{"Y.5.60_seed=2",45},
		{"Y.5.60_seed=3",34},
		{"Y.5.60_seed=4",48},
		{"Y.5.60_seed=5",40},
		{"Y.5.80_seed=1",45},
		{"Y.5.80_seed=2",59},
		{"Y.5.80_seed=3",44},
		{"Y.5.80_seed=4",63},
		{"Y.5.80_seed=5",53},
		{"Y.5.100_seed=1",56},
		{"Y.5.100_seed=2",73},
		{"Y.5.100_seed=3",54},
		{"Y.5.100_seed=4",77},
		{"Y.5.100_seed=5",66},
		{"Z.4x25.20",66},
		{"Z.4x25.40",127},
		{"Z.4x25.60",192},
		{"Z.4x25.80",258},
		{"z.4x25.100",312},
		{"z.5x20.20",54},
		{"z.5x20.40",101},
		{"z.5x20.60",154},
		{"z.5x20.80",205},
		{"z.5x20.100",251},
		{"z.6x17.20",44},
		{"z.6x17.40",85},
		{"z.6x17.60",129},
		{"z.6x17.80",171},
		{"z.6x17.100",216},
		{"z.8x13.20",33},
		{"z.8x13.40",64},
		{"z.8x13.60",97},
		{"z.8x13.80",130},
		{"z.8x13.100",169},
		{"z.10x10.20",28},
		{"z.10x10.40",54},
		{"z.10x10.60",82},
		{"z.10x10.80",108},
		{"z.10x10.100",133},*/
		});
	void Simulator::initDefaultEnvironment() {
		RWAsolver::Environment env;
		env.save(Env::DefaultEnvPath());
		RWAsolver::Configuration cfg;
	}

	void Simulator::run(const Task &task) {
		String instanceName(task.instSet + task.instId + ".json");
		String solutionName(task.instSet + task.instId + ".json");

		char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
		char *argv[Cmd::MaxArgNum];
		for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
		strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

		int argc = ArgIndex::ArgStart;

		strcpy(argv[argc++], Cmd::InstancePathOption().c_str());
		strcpy(argv[argc++], (InstanceDir() + instanceName).c_str());

		System::makeSureDirExist(SolutionDir());
		strcpy(argv[argc++], Cmd::SolutionPathOption().c_str());
		strcpy(argv[argc++], (SolutionDir() + solutionName).c_str());

		if (!task.currentBest.empty()) {
			strcpy(argv[argc++], Cmd::CurrentBestOption().c_str());
			strcpy(argv[argc++], task.currentBest.c_str());
		}

		if (!task.randSeed.empty()) {
			strcpy(argv[argc++], Cmd::RandSeedOption().c_str());
			strcpy(argv[argc++], task.randSeed.c_str());
		}

		if (!task.timeout.empty()) {
			strcpy(argv[argc++], Cmd::TimeoutOption().c_str());
			strcpy(argv[argc++], task.timeout.c_str());
		}

		if (!task.maxIter.empty()) {
			strcpy(argv[argc++], Cmd::MaxIterOption().c_str());
			strcpy(argv[argc++], task.maxIter.c_str());
		}

		if (!task.jobNum.empty()) {
			strcpy(argv[argc++], Cmd::JobNumOption().c_str());
			strcpy(argv[argc++], task.jobNum.c_str());
		}

		if (!task.runId.empty()) {
			strcpy(argv[argc++], Cmd::RunIdOption().c_str());
			strcpy(argv[argc++], task.runId.c_str());
		}

		if (!task.cfgPath.empty()) {
			strcpy(argv[argc++], Cmd::ConfigPathOption().c_str());
			strcpy(argv[argc++], task.cfgPath.c_str());
		}

		if (!task.logPath.empty()) {
			strcpy(argv[argc++], Cmd::LogPathOption().c_str());
			strcpy(argv[argc++], task.logPath.c_str());
		}

		if (!task.perturbInterval.empty()) {
			strcpy(argv[argc++], Cmd::PerturbInterval().c_str());
			strcpy(argv[argc++], task.perturbInterval.c_str());
		}

		if (!task.perturbTprob.empty()) {
			strcpy(argv[argc++], Cmd::PerturbTprob().c_str());
			strcpy(argv[argc++], task.perturbTprob.c_str());
		}

		if (!task.perturbNum.empty()) {
			strcpy(argv[argc++], Cmd::PerturbNum().c_str());
			strcpy(argv[argc++], task.perturbNum.c_str());
		}


		if (!task.worseSolProb.empty()) {
			strcpy(argv[argc++], Cmd::WorseSolProb().c_str());
			strcpy(argv[argc++], task.worseSolProb.c_str());
		}

		if (!task.swithScale.empty()) {
			strcpy(argv[argc++], Cmd::SwithScale().c_str());
			strcpy(argv[argc++], task.swithScale.c_str());
		}

		if (!task.mBest.empty()) {
			strcpy(argv[argc++], Cmd::MBest().c_str());
			strcpy(argv[argc++], task.mBest.c_str());
		}
		Cmd::run(argc, argv);
	}

	void Simulator::run(const String &envPath) {
		char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
		char *argv[Cmd::MaxArgNum];
		for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
		strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

		int argc = ArgIndex::ArgStart;

		strcpy(argv[argc++], Cmd::EnvironmentPathOption().c_str());
		strcpy(argv[argc++], envPath.c_str());

		Cmd::run(argc, argv);
	}

	void Simulator::debug() {
		Task task;
		task.instSet = "z.8x13.20";
		task.currentBest = "33";
		task.perturbInterval = "850";
		task.perturbTprob = "80";
		task.perturbNum = "4";
		task.worseSolProb = "10";
		task.swithScale = "2";
		task.mBest = "6";
		task.instId = "";
		task.randSeed = "1583376036";
		//task.randSeed = to_string(RandSeed::generate());	
		task.timeout = "180";
		//task.maxIter = "1000000000";
		task.jobNum = "1";
		task.cfgPath = Env::DefaultCfgPath();
		task.logPath = Env::DefaultLogPath();
		task.runId = "0";

		run(task);
	}

	void Simulator::benchmark(int repeat) {
		Task task;
		task.instSet = "";
		//task.timeout = "180";
		//task.maxIter = "1000000000";
		task.timeout = "3600";
		//task.maxIter = "1000000000";
		task.jobNum = "1";
		task.cfgPath = Env::DefaultCfgPath();
		task.logPath = Env::DefaultLogPath();

		random_device rd;
		mt19937 rgen(rd());
		for (int i = 0; i < repeat; ++i) {
			//shuffle(instList.begin(), instList.end(), rgen);
			String seed = to_string(Random::generateSeed());
			//seed = "1583155699";
			for (auto pi = 850; pi <= 850; pi += 50) {
				for (auto pt = 80; pt < 81; ++pt) {
					for (auto pn = 4; pn < 5; ++pn) {
						for (auto wsp = 10; wsp < 11; wsp+=5) {
							for (auto ss = 75; ss <= 75; ss+=5) {
								for (auto mb = 2; mb <= 2; mb++)
								{
									task.perturbInterval = to_string(pi);
									task.perturbTprob = to_string(pt);
									task.perturbNum = to_string(pn);
									task.worseSolProb = to_string(wsp);
									task.swithScale = to_string(ss);
									task.mBest = to_string(mb);
									auto sw = startWave.begin();
									for (auto inst = instList.begin(); inst != instList.end(); ++inst, ++sw) {
										task.instId = (*inst).first;
										int currentBest = (*sw).second;
										int lowerBound = (*inst).second;
										//while(currentBest >= lowerBound){
										task.currentBest = to_string(currentBest);
										task.randSeed = seed;
										task.runId = to_string(i);
										std::cout << "Searching the waveNum is " << currentBest << endl;
										clock_t start = clock();
										run(task);
										double t = (clock() - start)*1.0 / CLOCKS_PER_SEC;
										currentBest--;
										//}

									}
								}
								
							}
							
						}

					}

				}
			}

		}
	}

	void Simulator::parallelBenchmark(int repeat) {
		Task task;
		task.instSet = "";
		//task.timeout = "180";
		//task.maxIter = "1000000000";
		task.timeout = "3600";
		//task.maxIter = "1000000000";
		task.jobNum = "1";
		task.cfgPath = Env::DefaultCfgPath();
		task.logPath = Env::DefaultLogPath();

		ThreadPool<> tp(50);

		random_device rd;
		mt19937 rgen(rd());
		for (int i = 0; i < repeat; ++i) {
			//shuffle(instList.begin(), instList.end(), rgen);
			for (auto pi = 850; pi < 860; pi += 10) {
				for (auto pt = 70; pt < 81; ++pt) {
					for (auto pn = 3; pn < 7; ++pn) {
						for (auto wsp = 3; wsp < 20; ++wsp) {

							task.perturbInterval = to_string(pi);
							task.perturbTprob = to_string(pt);
							task.perturbNum = to_string(pn);
							task.worseSolProb = to_string(wsp);
							auto sw = startWave.begin();
							for (auto inst = instList.begin(); inst != instList.end(); ++inst, ++sw) {
								task.instId = (*inst).first;
								int currentBest = (*sw).second;
								int lowerBound = (*inst).second;

								//while (currentBest >= lowerBound) {
								task.currentBest = to_string(lowerBound);
								task.randSeed = to_string(Random::generateSeed());
								task.runId = to_string(i);
								std::cout << "the waveNum is " << currentBest << endl;
								tp.push([=]() { run(task); });
								this_thread::sleep_for(1s);
								currentBest--;
								//}
							}
						}
					}
				}
			}
		}
	}

	void Simulator::parallelBenchmark_withpara(int repeat)
	{
		Task task;
		task.instSet = "";
		//task.timeout = "180";
		//task.maxIter = "1000000000";
		task.timeout = "3600";
		//task.maxIter = "1000000000";
		task.jobNum = "1";
		task.cfgPath = Env::DefaultCfgPath();
		task.logPath = Env::DefaultLogPath();

		ThreadPool<> tp(50);

		random_device rd;
		mt19937 rgen(rd());
		for (int i = 0; i < repeat; ++i) {
			//shuffle(instList.begin(), instList.end(), rgen);
			for (auto j = 0; j < parameter.size(); ++j) {
				ID pi = parameter[j][0];
				ID pt = parameter[j][1];
				ID pn = parameter[j][2];
				ID wsp = parameter[j][3];
				task.perturbInterval = to_string(pi);
				task.perturbTprob = to_string(pt);
				task.perturbNum = to_string(pn);
				task.worseSolProb = to_string(wsp);
				auto sw = startWave.begin();
				for (auto inst = instList.begin(); inst != instList.end(); ++inst, ++sw) {
					task.instId = (*inst).first;
					int currentBest = (*sw).second;
					int lowerBound = (*inst).second;

					//while (currentBest >= lowerBound) {
					task.currentBest = to_string(lowerBound);
					task.randSeed = to_string(Random::generateSeed());
					task.runId = to_string(i);
					std::cout << "the waveNum is " << currentBest << endl;
					tp.push([=]() { run(task); });
					this_thread::sleep_for(1s);
					currentBest--;
					//}
				}
			}		
		}
	}

	
	void Simulator::convertYandZInstance(const String trfPath, const String netPath, const String table) {

		ifstream trf(InstanceDir() + table + trfPath + ".trf");
		ifstream net(InstanceDir() + table + netPath + ".net");

		int nodeNum, edgeNum;
		net >> nodeNum >> edgeNum;

		Arr2D<int> edgeIndices(nodeNum, nodeNum, -1);

		Problem::Input input;

		auto &graph(*input.mutable_graph());
		graph.set_nodenum(nodeNum);
		for (int e = 0; e < edgeNum; ++e) {
			int source, target;
			net >> source >> target;

			if (source > target) { swap(source, target); }

			if (edgeIndices.at(source, target) < 0) {
				edgeIndices.at(source, target) = graph.edges().size();
				auto &edge(*graph.add_edges());
				edge.set_source(source);
				edge.set_target(target);
			}
		}

		int trafficNum;
		trf >> trafficNum;
		for (auto t = 0; t < trafficNum; ++t) {
			int src, dst;
			trf >> src >> dst;
			auto &traffics(*input.add_traffics());
			traffics.set_src(src);
			traffics.set_dst(dst);
			traffics.set_id(t);
		}

		ostringstream outPath;
		outPath << InstanceDir() << trfPath + ".json";
		save(outPath.str(), input);
	}


}
