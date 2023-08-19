#include "RWAsolver.h"

using namespace std;
namespace szx {
	List<int> binValueCnt;
#pragma region RWAsolver::Cli
	int RWAsolver::Cli::run(int argc, char * argv[]) {
		Log(LogSwitch::Szx::Cli) << "parse command line arguments." << std::endl;
		Set<String> switchSet;
		Map<String, char*> optionMap({ // use string as key to compare string contents instead of pointers.
			{ InstancePathOption(), nullptr },
			{ SolutionPathOption(), nullptr },
			{CurrentBestOption(),nullptr},
			{ RandSeedOption(), nullptr },
			{ TimeoutOption(), nullptr },
			{ MaxIterOption(), nullptr },
			{ JobNumOption(), nullptr },
			{ RunIdOption(), nullptr },
			{ EnvironmentPathOption(), nullptr },
			{ ConfigPathOption(), nullptr },
			{ LogPathOption(), nullptr },
			{ PerturbInterval(), nullptr },
			{ PerturbTprob(), nullptr },
			{ PerturbNum(), nullptr },
			{ WorseSolProb(), nullptr },
			{ SwithScale(), nullptr },
			{ MBest(), nullptr }
			});

		for (int i = 1; i < argc; ++i) { // skip executable name.
			auto mapIter = optionMap.find(argv[i]);
			if (mapIter != optionMap.end()) { // option argument.
				mapIter->second = argv[++i];
			}
			else { // switch argument.
				switchSet.insert(argv[i]);
			}
		}

		Log(LogSwitch::Szx::Cli) << "execute commands." << endl;
		if (switchSet.find(HelpSwitch()) != switchSet.end()) {
			cout << HelpInfo() << endl;
		}

		if (switchSet.find(AuthorNameSwitch()) != switchSet.end()) {
			cout << AuthorName() << endl;
		}

		RWAsolver::Environment env;
		env.load(optionMap);
		if (env.instPath.empty() || env.slnPath.empty()) { return -1; }

		RWAsolver::Configuration cfg;

		Log(LogSwitch::Szx::Input) << "load instance " << env.instPath << " (seed=" << env.randSeed << ")." << endl;
		Problem::Input input;
		if (!input.load(env.instPath)) { return -1; }

		RWAsolver solver(input, env, cfg);
		solver.solve();
		//if (solver.conflictsNum == 0) {

			pb::Submission submission;
			submission.set_thread(to_string(env.jobNum));
			submission.set_instance(env.friendlyInstName());
			submission.set_duration(to_string(solver.timer.elapsedSeconds()) + "s");
			solver.output.save(env.slnPath, submission);
#if SZX_DEBUG
			solver.output.save(env.solutionPathWithTime(), submission);
			solver.record();
#endif // SZX_DEBUG
		//}
		

		return 0;
	}
#pragma endregion RWAsolver::Cli
#pragma region RWAsolver::Environment
	void RWAsolver::Environment::load(const Map<String, char*> &optionMap) {
		char *str;

		str = optionMap.at(Cli::EnvironmentPathOption());
		if (str != nullptr) { loadWithoutCalibrate(str); }

		str = optionMap.at(Cli::InstancePathOption());
		if (str != nullptr) { 
			instPath = str; 
			insName = str;
			int pos = insName.find(DefaultInstanceDir());
			int len = DefaultInstanceDir().length();
			if (pos > -1) {
				insName.erase(pos, len);
			}
			pos = insName.find(".json");
			len = 5;
			if (pos > -1) {
				insName.erase(pos, len);
			}
		}

		str = optionMap.at(Cli::SolutionPathOption());
		if (str != nullptr) { slnPath = str; }

		str = optionMap.at(Cli::CurrentBestOption());
		if (str != nullptr) { currentBestValue = atoi(str); }

		str = optionMap.at(Cli::RandSeedOption());
		if (str != nullptr) { randSeed = atoi(str); }

		str = optionMap.at(Cli::TimeoutOption());
		if (str != nullptr) { msTimeout = static_cast<Duration>(atof(str) * Timer::MillisecondsPerSecond); }

		str = optionMap.at(Cli::MaxIterOption());
		if (str != nullptr) { maxIter = atoi(str); }

		str = optionMap.at(Cli::JobNumOption());
		if (str != nullptr) { jobNum = atoi(str); }

		str = optionMap.at(Cli::RunIdOption());
		if (str != nullptr) { rid = str; }

		str = optionMap.at(Cli::ConfigPathOption());
		if (str != nullptr) { cfgPath = str; }

		str = optionMap.at(Cli::LogPathOption());
		if (str != nullptr) { logPath = str; }

		str = optionMap.at(Cli::PerturbInterval());
		if (str != nullptr) { perturbInterval = atoi(str); }

		str = optionMap.at(Cli::PerturbTprob());
		if (str != nullptr) { perturbTprob =atoi(str); }

		str = optionMap.at(Cli::PerturbNum());
		if (str != nullptr) { perturbNum = atoi(str); }

		str = optionMap.at(Cli::WorseSolProb());
		if (str != nullptr) { worseSolProb = atoi(str); }

		str = optionMap.at(Cli::SwithScale());
		if (str != nullptr) { swithScale = atoi(str); }

		str = optionMap.at(Cli::MBest());
		if (str != nullptr) { mBest = atoi(str); }
		calibrate();
	}
	static bool probChoice(double r) {
		double temp = (double)rand() / (double)(RAND_MAX + 1);
		return temp < r;
	}
	static bool cntCentProbChoice(int cnt) {
		double r = 1.0 / cnt;
		double temp = (double)rand() / (double)(RAND_MAX + 1);
		return temp < r;
	}
	void RWAsolver::Environment::load(const String &filePath) {
		loadWithoutCalibrate(filePath);
		calibrate();
	}

	void RWAsolver::Environment::loadWithoutCalibrate(const String &filePath) {
		// EXTEND[szx][8]: load environment from file.
		// EXTEND[szx][8]: check file existence first.
	}

	void RWAsolver::Environment::save(const String &filePath) const {
		// EXTEND[szx][8]: save environment to file.
	}
	void RWAsolver::Environment::calibrate() {
		// adjust thread number.
		int threadNum = thread::hardware_concurrency();
		if ((jobNum <= 0) || (jobNum > threadNum)) { jobNum = threadNum; }

		// adjust timeout.
		msTimeout -= Environment::SaveSolutionTimeInMillisecond;
	}
#pragma endregion RWAsolver::Environment
	 List<ID> getPath(List<List<ID>> &nextID, ID src, ID dst) {
		List<ID> path;
		ID temp = src;
		path.push_back(src);
		while (temp != dst) {
			temp = nextID[temp][dst];
			path.push_back(temp);
		}
		return path;
	}

	void printPath(List<ID> &path) {
		for (auto i = 0; i < path.size(); ++i) {
			std::cout << path[i] << "  ";
		}
		std::cout << "\n";
	}

	bool RWAsolver::solve()
	{
		//init();

		int workerNum = (max)(1, env.jobNum / cfg.threadNumPerWorker);
		cfg.threadNumPerWorker = env.jobNum / workerNum;
		List<Solution> solutions(workerNum, Solution(this));
		List<bool> success(workerNum);

		Log(LogSwitch::Szx::Framework) << "launch " << workerNum << " workers." << endl;
		List<thread> threadList;
		threadList.reserve(workerNum);
		for (int i = 0; i < workerNum; ++i) {
			threadList.emplace_back([&, i]() { success[i] = optimize(solutions[i], i); });
		}
		for (int i = 0; i < workerNum; ++i) { threadList.at(i).join(); }

		Log(LogSwitch::Szx::Framework) << "collect best result among all workers." << endl;
		int bestIndex = -1;
		Length bestValue = Problem::MaxDistance;
		for (int i = 0; i < workerNum; ++i) {
			if (!success[i]) { continue; }
			Log(LogSwitch::Szx::Framework) << "worker " << i << " got " << solutions[i].waveLengthNum << endl;
			if (solutions[i].waveLengthNum >= bestValue) { continue; }
			bestIndex = i;
			bestValue = solutions[i].waveLengthNum;
		}

		env.rid = to_string(bestIndex);
		if (bestIndex < 0) { return false; }
		output = solutions[bestIndex];
		return true;
	}

	bool RWAsolver::check(Length & checkerObj) const
	{
#if SZX_DEBUG
		enum CheckerFlag {
			IoError = 0x0,
			FormatError = 0x1,
			TooManyCentersError = 0x2
		};

		checkerObj = System::exec("Checker.exe " + env.instPath + " " + env.solutionPathWithTime());
		if (checkerObj > 0) { return true; }
		checkerObj = ~checkerObj;
		if (checkerObj == CheckerFlag::IoError) { Log(LogSwitch::Checker) << "IoError." << endl; }
		if (checkerObj & CheckerFlag::FormatError) { Log(LogSwitch::Checker) << "FormatError." << endl; }
		if (checkerObj & CheckerFlag::TooManyCentersError) { Log(LogSwitch::Checker) << "TooManyCentersError." << endl; }
		return false;
#else
		checkerObj = 0;
		return true;
#endif // SZX_DEBUG
	}

	void RWAsolver::record() const
	{
#if SZX_DEBUG
		int generation = 0;

		ostringstream log;

		System::MemoryUsage mu = System::peakMemoryUsage();

		Length obj = output.waveLengthNum;
		Length checkerObj = -1;
		bool feasible = 1;
		feasible = check(checkerObj);

		// record basic information.
		log << env.friendlyLocalTime() << ","
			<< env.rid << ","
			<< env.instPath << ","
			<< feasible << "," << (obj - checkerObj) << ",";
		log << obj << ",";
		log << duration_time << ","
			<< mu.physicalMemory << "," << mu.virtualMemory << ","
			<< env.randSeed << ","
			<< cfg.toBriefStr() << ","
			<< generation << "," << iteration << ","<<perturbInterval<<","<<perturbGreedy_tprob<<","<<perturbNum<<","<<env.worseSolProb<<","<<env.swithScale<<","<<env.mBest;

		/*for (auto t = output.traout().begin(); t != output.traout().end(); ++t) {
			log << "id: " << (*t).id() << " ";
			log << "original_w:"<<(*t).wave()<<"  p: ";
			for (auto o = (*t).path().begin(); o != (*t).path().end(); ++o) {
				log << *o << " ";
			}
			log << ";";
		}*/
		log << endl;

		// append all text atomically.
		static mutex logFileMutex;
		lock_guard<mutex> logFileGuard(logFileMutex);

		ofstream logFile(env.logPath, ios::app);
		logFile.seekp(0, ios::end);
		if (logFile.tellp() <= 0) {
			logFile << "Time,ID,Instance,Feasible,ObjMatch,WaveNum,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration,Solution" << endl;
		}
		logFile << log.str();
		logFile.close();
#endif // SZX_DEBUG
	}
	void RWAsolver::switchNeighType(NeighBorType & neighType)
	{
		if (neighType == NeighBorType_Shift)
			neighType = NeighBorType_Exchange;
		else
			neighType = NeighBorType_Shift;
	}
	void shuffleTraffics(List<Traffic *> &temp_traffics, int start, int end) {
		int size = end - start + 1;
		for (auto i = start; i < end; ++i) {
			int t = i + rand() % (size - (i - start));
			swap(temp_traffics[i], temp_traffics[t]);
		}
	}
	
	bool RWAsolver::optimize(Solution & sln, ID workerId)
	{
		// TODO[optimize]: 添加自己的算法
		// 输入和输出接口请查看 /lib/RWA.proto

		init();
		clock_t c1 = clock();
		bfd_init_solution();
		//WaveNum = env.currentBestValue + 1;
		
		dij_cnt = 0;
		process.clear();
		int real_perturbInterval = perturbInterval;
		do {
			if (conflictsNum == 0) {
				if (waveGroups.size() != 0) {

					for (auto w = 0; w < WaveNum; ++w) {

						for (auto iter = waveGroups[w].tras.begin(); iter != waveGroups[w].tras.end(); ++iter) {
							ID id = (*iter)->id;
							traffics[id].wave = (*iter)->wave;
							traffics[id].nodes = (*iter)->nodes;

						}
					}
					clock_t c2 = clock() - c1;
					duration_time = c2 * 1.0 / CLOCKS_PER_SEC;
					sln.clear_traout();
					for (auto i = 0; i < traffics.size(); ++i) {
						auto &traout(*sln.add_traout());
						traout.set_wave(traffics[i].wave);
						traout.set_id(traffics[i].id);
						for (auto j = 0; j < traffics[i].nodes.size(); ++j) {
							traout.add_path(traffics[i].nodes[j]);
						}
					}
					sln.waveLengthNum = WaveNum;
					/*String filename = "box_results//" + env.insName + "_result" + to_string(WaveNum) + ".txt";
					myOutput.save(traffics, 0, filename);*/
				}
				
			}
			WaveNum--;
			if (WaveNum < env.currentBestValue) break;
			cout << "current searching waveNum is " << WaveNum << endl;
			init_solution();
			iteration = 0;
			clock_t c2 = clock() - c1;
			double time;
			if (WaveNum == env.currentBestValue) {

				time = c2 * 1.0 / CLOCKS_PER_SEC;
				process.push_back(make_pair(time, conflictsNum));
			}
			noBeterTimes = 1;
			int sameCnt = 1;
			neighType = NeighBorType_Shift;
			//neighType = NeighBorType_Exchange;
			while (conflictsNum != 0) {
				Move move;
				//layeredFind_move_all_by_dijkstra(); // SD-ILS with the evaluation results cache 
				//layeredFind_move_all_by_ejection();
				//layeredFind_move();
				//layeredFind_move_m_best();
				//merged_shift_exchange();

				/*if (neighType == NeighBorType_Shift) {
					layeredFind_move_m_best_original();
				}
				else
					exchangeTraffic_all();*/

				// Shift+Swap neighbor
				if( rand()%100 < env.swithScale)
					layeredFind_move_m_best_original();
				else
					exchangeTraffic_all();
				
				

				if (conflictsNum < best_conflictsNum) {
					c2 = clock() - c1;
					if (WaveNum == env.currentBestValue) {
						time = c2 * 1.0 / CLOCKS_PER_SEC;
						process.push_back(make_pair(time, conflictsNum));
					}
					best_conflictsNum = conflictsNum;
					noBeterTimes = 0;
					sameCnt = 1;

					/*cout << "new conflictNum:" << best_conflictsNum;
					if (neighType == NeighBorType_Shift) cout << " shift" << endl;
					else cout << " exchange" << endl;*/
					updateBestSolution();
				}
				else {
					noBeterTimes++;
					if (conflictsNum == best_conflictsNum) {
						sameCnt++;
						if (sameCnt >= 200) {
							updateBestSolution();
							sameCnt = 0;
						}
					}
				}
				if (noBeterTimes == real_perturbInterval) {
					perturb_greedy();
					noBeterTimes = 0;
				}
				if (noBeterTimes == real_perturbInterval /env.swithScale ) {
					switchNeighType(neighType);
				}
				iteration++;
				c2 = clock() - c1;
				time = c2 * 1.0 / CLOCKS_PER_SEC;
				if (time > 300) break;
			}

			
		} while (WaveNum >= env.currentBestValue);
		cout << "dij number = " << dij_cnt<<endl;
		cout << "evalute cnt = " << evaluate_cnt << endl;
		ofstream ofs;
		ofs.open("./csv-process/" + env.insName + ".csv", ios::out);
		for (size_t i = 0; i < process.size(); i++)
		{
			ofs << process[i].first << "," << process[i].second << endl;
		}
		
		return true;
	}

	void RWAsolver::updateBestSolution()
	{
		best_conflictsNum = conflictsNum;
		for (auto w = 0; w < WaveNum; ++w) {
			bestWaveGroups[w] = waveGroups[w];
			List<Traffic *> &s = bestWaveGroups[w].tras;
			for (auto iter = s.begin(); iter != s.end(); ++iter) {
				Traffic *tra = *iter;
				tra->bestSol.bestNodes = tra->nodes;
				tra->bestSol.wave = tra->wave;
			}
		}
		bestConflicTras = conflictTras;
		best_groupChangedIter = groupChangedIter;
		best_recordIter = recordIter;
		best_leaveTime = leaveTime;
		best_delta = delta;
		recordBestIter = iteration;
	}



	void RWAsolver::perturb()
	{
		
		if (cntCentProbChoice(3) &&conflictsNum <= best_conflictsNum ) {//小概率从局部最优解进行扰动，大概率从历史最优解进行扰动
			
		}
		else 
		{// 从最优解中恢复
			conflictsNum = best_conflictsNum;

			for (auto w = 0; w < WaveNum; ++w) {
				if (recordBestIter > groupChangedIter[w])continue;
				waveGroups[w] = bestWaveGroups[w];
				List<Traffic *> &s = waveGroups[w].tras;

				for (auto iter = s.begin(); iter != s.end(); ++iter) {
					Traffic *tra = *iter;
					tra->nodes = tra->bestSol.bestNodes;
					tra->wave = tra->bestSol.wave;
				}
				//waveGroups[w].afreshAdjMat(topo.adjMat);
				//waveGroups[w].recordConflictTras(topo, conflictTras);
			}
			groupChangedIter = best_groupChangedIter;
			delta = best_delta;
			recordIter = best_recordIter;
			leaveTime = best_leaveTime;
			conflictTras = bestConflicTras;
		}
		check_all();
		//进行扰动
		int Tprob;
		if (best_conflictsNum > 30)
			Tprob = 9;
		else if (best_conflictsNum > 20)
			Tprob = 8;
		else if (best_conflictsNum > 10)
			Tprob = 7;
		else if (best_conflictsNum > 5)
			Tprob = 6;
		else if (best_conflictsNum > 3)
			Tprob = 3;
		else
			Tprob = 0;
		List<bool> changed;
		changed.resize(WaveNum);
		for (auto w = 0; w < WaveNum; ++w) {
			if (rand() % 10 < Tprob)
				continue;
			WaveGroup &group = waveGroups[w];
			int groupConfNum = group.groupConflicTras.Num;
			if (groupConfNum == 0) continue;
			int randsel = rand() % groupConfNum;
			Traffic *traA = &traffics[group.groupConflicTras.data[randsel]];
			int w2;
			do {
				w2 = rand() % WaveNum;
			} while (w2 == w );
			WaveGroup &group2 = waveGroups[w2];
			changed[w] = true;
			changed[w2] = true;
			if (group2.groupConflicTras.Num == 0) {
				conflictsNum-= group.deleteTra(traA, conflictTras);
				//dijkstra(waveGroups[w2].adjMatTraNums, traA, traA->nodes);
				conflictsNum+= group2.addTra(traA);
				traA->wave = w2;
				/*if (conflictsNum <= 5) {
					int randselB = rand() % group2.tras.size();
					Traffic *traB = group2.tras[randselB];
					traB->wave = w;

					conflictsNum -= group2.deleteTra(traB, conflictTras);
					conflictsNum += group.addTra(traB);
				}*/
			}
			else {

				int groupConfNumB = group2.groupConflicTras.Num;
				int randselB = rand() % groupConfNumB;
				Traffic *traB = &traffics[ group2.groupConflicTras.data[randselB]];
				traA->wave = w2;
				traB->wave = w;
				conflictsNum -= group.deleteTra(traA, conflictTras);
				conflictsNum -= group2.deleteTra(traB, conflictTras);
				int delta;
				//dijkstra(waveGroups[w2].adjMatTraNums, traA, traA->nodes);
				//dijkstra(waveGroups[w].adjMatTraNums, traB, traB->nodes);
				conflictsNum += group.addTra(traB);
				conflictsNum += group2.addTra(traA);

			}
		}
		for (auto w = 0; w < WaveNum; ++w) {
			if (changed[w]) {
				groupChangedIter[w] = iteration;
				waveGroups[w].recordConflictTras(topo, conflictTras);
			}
		}
		check_all();
	}
	bool TrafficPointerCompare(const Traffic *tra1, const Traffic *tra2) {
		return tra1->shorestNodes.size() > tra2->shorestNodes.size();
	}
	bool TrafficPointerCompareNodes(const Traffic *tra1, const Traffic *tra2) {
		return tra1->shorestNodes.size() > tra2->shorestNodes.size();
	}
	bool TrafficCompare(const Traffic &tra1, const Traffic &tra2) {
		return tra1.shorestNodes.size() > tra2.shorestNodes.size();
	}
	void RWAsolver::perturb_greedy()
	{
		if (cntCentProbChoice(3) && conflictsNum <= best_conflictsNum) {//小概率从局部最优解进行扰动，大概率从历史最优解进行扰动

		}
		{// 从最优解中恢复
			conflictsNum = best_conflictsNum;

			for (auto w = 0; w < WaveNum; ++w) {
				if (recordBestIter > groupChangedIter[w])continue;
				waveGroups[w] = bestWaveGroups[w];
				List<Traffic *> &s = waveGroups[w].tras;

				for (auto iter = s.begin(); iter != s.end(); ++iter) {
					Traffic *tra = *iter;
					tra->nodes = tra->bestSol.bestNodes;
					tra->wave = tra->bestSol.wave;
				}
			}
			groupChangedIter = best_groupChangedIter;
			delta = best_delta;
			recordIter = best_recordIter;
			leaveTime = best_leaveTime;
			conflictTras = bestConflicTras;
		}

		List<Traffic*> traPs; 
		List<bool> changed;
		changed.resize(WaveNum);
		traPs.reserve(conflictTras.Num);
		for (auto i = 0; i < conflictTras.Num; ++i) {
			traPs.push_back(&traffics[conflictTras.data[i]]);
		}
		/*if (conflictsNum <= 3) {
			perturbGreedy_tprob = 5;
		}*/
		shuffleTraffics(traPs, 0, traPs.size()-1);
		int perturbTraNum = min(perturbNum,(int)traPs.size());
		//partial_sort(traPs.begin(), traPs.begin() + perturbNum,traPs.end(), TrafficCompare);
		//sort(traPs.begin(), traPs.end(), TrafficPointerCompare);
		for (auto i = 0; i < perturbTraNum; ++i) {
			Traffic *tra = traPs[i];
			int original_w = tra->wave;
			changed[original_w] = true;
			conflictsNum-= waveGroups[original_w].deleteTra(tra,conflictTras);
		}
		for (auto i = 0; i < perturbTraNum; ++i) {
			Traffic *tra = traPs[i];
			int original_w = tra->wave;
			List<ID> temp_nodes;
			int temp_delt;
			int best_delt = INT_MAX;
			int cnt = 1;
			int bestWave;
			List<ID> best_nodes;
			perturbRandomCnt = 3;
			if (cntCentProbChoice(perturbRandomCnt)) {
				int w = rand() % WaveNum;
				dijkstraMinConflicts(waveGroups[w].adjMatTraNums, tra, best_delt, best_nodes);
				bestWave = w;
			}
			else {
				for (auto w = 0; w < WaveNum; ++w) {
					if (rand() % 100 < perturbGreedy_tprob)
						continue;
					dijkstraMinConflicts(waveGroups[w].adjMatTraNums, tra, temp_delt, temp_nodes);
					if (temp_delt < best_delt) {
						best_delt = temp_delt;
						bestWave = w;
						best_nodes = temp_nodes;
						cnt = 1;
					}
					else if (temp_delt == best_delt) {
						cnt++;
						if (cntCentProbChoice(cnt)) {
							bestWave = w;
							best_nodes = temp_nodes;
						}
					}
				}
			}
			if (best_nodes.size() != 0) {
				tra->nodes = best_nodes;
				tra->wave = bestWave;
				leaveTime[tra->id][original_w] = iteration;
				changed[bestWave] = true;
				conflictsNum += waveGroups[bestWave].addTra(tra);
			}
			else {
				conflictsNum += waveGroups[tra->wave].addTra(tra);
			}
		}
		for (auto w = 0; w < WaveNum; ++w) {
			if (changed[w]) {
				groupChangedIter[w] = iteration;
				waveGroups[w].recordConflictTras(topo, conflictTras);
			}
		}
	}

	void RWAsolver::init()
	{
		srand(env.randSeed);
		auto &graph(*input.mutable_graph());
		auto &edges(*graph.mutable_edges());
		auto &itraffics(*input.mutable_traffics());
		nodeNum = graph.nodenum();
		linkNum = edges.size();

		topo.nextID.resize(nodeNum);
		topo.adjLinkMat.resize(nodeNum);
		topo.linkNodes.resize(nodeNum);
		topo.adjMat.init(nodeNum, nodeNum);
		std::fill(topo.adjMat.begin(), topo.adjMat.end(), MaxDistance);
		for (auto i = 0; i < nodeNum; ++i) {
			topo.nextID[i].resize(nodeNum,InvalidID);
			topo.adjLinkMat[i].resize(nodeNum,InvalidID);
		}

		int linkID = 0;
		for (auto e = edges.begin(); e != edges.end(); ++e) {
			int u, v;
			u = (*e).source();
			v = (*e).target();

			topo.nextID[u][v] = v;
			topo.adjLinkMat[u][v] = linkID++;
			topo.adjMat.at(u, v) = 1;
			topo.linkNodes[u].push_back(v);

			topo.nextID[v][u] = u;
			topo.adjLinkMat[v][u] = linkID++;
			topo.adjMat.at(v, u) = 1;
			topo.linkNodes[v].push_back(u);
		}
		Arr2D<Length> tempAdjMat;
		tempAdjMat.init(nodeNum, nodeNum);
		for (auto i = 0; i < nodeNum; ++i) {
			for (auto j = 0; j < nodeNum; ++j) {
				tempAdjMat[i][j] = topo.adjMat[i][j];
			}
		}
		Floyd::findAllPairsPaths_asymmetric(tempAdjMat, topo.nextID);

		trafficNum = itraffics.size();
		for (auto t = itraffics.begin(); t != itraffics.end(); ++t) {
			int src, dst,id;
			src = (*t).src();
			dst = (*t).dst();
			id = (*t).id();

			traffics.push_back(Traffic(id, src, dst));
		}
		ptraffics.resize(traffics.size());
		for (auto i = 0; i < trafficNum; i++){
			ptraffics[i] = &traffics[i];
		}
		List<List<bool>> overlapMat;
		overlapMat.resize(trafficNum);
		for (auto i = 0; i < trafficNum; ++i) {
			overlapMat[i].resize(trafficNum);
			traffics[i].shorestNodes = getPath(topo.nextID, traffics[i].src, traffics[i].dst);
		}
		//printPath(getPath(topo.nextID, 1, 4));
		WaveNum = env.currentBestValue;
		delBoxchangeTras.initSpace(trafficNum);
		best_newTrasNodes.initSpace(trafficNum);
		best_delBoxchangeTras.initSpace(trafficNum);
		currentNewTrasNodes.initSpace(trafficNum);
		tempNewNodes.initSpace(trafficNum);
		tempNewNode2.initSpace(trafficNum);
		evaluatedTras.initSpace(trafficNum);
		perturbInterval = env.perturbInterval;
		perturbGreedy_tprob = env.perturbTprob;
		perturbNum = env.perturbNum;
		binValueCnt.resize(30);
	}
	
	void RWAsolver::init_solution()
	{
		color_tabu_table.clear();
		delta.clear();
		recordIter.clear();
		leaveTime.clear();
		groupChangedIter.clear();
		color_tabu_table.resize(trafficNum);
		delta.resize(trafficNum);
		recordIter.resize(trafficNum);
		leaveTime.resize(trafficNum);
		for (auto i = 0; i < trafficNum; ++i) {
			color_tabu_table[i].resize(WaveNum);
			delta[i].resize(WaveNum);
			leaveTime[i].resize(WaveNum);
			recordIter[i].resize(WaveNum);
		}
		effectIterNum = 0;
		conflictTras.initSpace(trafficNum);
		bestConflicTras.initSpace(trafficNum);
		groupChangedIter.resize(WaveNum);

		sameSolutionCnt = 1;
		bestWaveGroups.clear();
		bestWaveGroups.reserve(WaveNum);
		for (auto w = 0; w < WaveNum; ++w) {
			WaveGroup waveGroup(topo.adjMat, trafficNum);
			bestWaveGroups.push_back(waveGroup);
		}
		bfd_decide_init_solution();
		//bfd_decide_init_solution_withEjectionChain();
		//deployByMemoryAndSubColor();
		//deployTrasByGraphColoring();
		//greedyDeployTras();
		//deployByFileAndSubColor();
#ifdef DEBUG_FY
		//检查业务数
		int trasNumInGroup = 0;
		for (auto w = 0; w < WaveNum; ++w) {
			trasNumInGroup += waveGroups[w].tras.size();
		}
		//printWaveGroups();
		if (trasNumInGroup != trafficNum) {
			cout << "trasNumInGroup error!\n";
		}
#endif // DEBUG_FY

		updateBestSolution();
	}
	
	void RWAsolver::bfd_init_solution() {
		List<Traffic *> temp_traffics;
		temp_traffics.resize(trafficNum);
		for (auto i = 0; i < trafficNum; ++i) {
			temp_traffics[i] = &traffics[i];
		}
		sort(temp_traffics.begin(), temp_traffics.end(), TrafficPointerCompare);
		/*int start = 0;
		while (true) {
			int end;
			int i;
			for (i = start; i < temp_traffics.size() - 1; ++i) {
				if (temp_traffics[i]->shorestNodes.size() != temp_traffics[i + 1]->shorestNodes.size()) {
					end = i;
					break;
				}
			}
			shuffleTraffics(temp_traffics, start, end);
			start = end + 1;
			if (i == temp_traffics.size() - 1)break;
		}*/
		
		int H = max((int)traffics[0].shorestNodes.size(),(int)sqrt(linkNum));

		int iter=0;
		WaveNum = INT_MAX;
		while (iter<=0)
		{
			List<WaveGroup> temp_wavegroups;
			temp_wavegroups.push_back(WaveGroup(topo.adjMat, trafficNum));
			for (auto i = 0; i < temp_traffics.size(); i++) {
				Traffic *tra = temp_traffics[i];
				int bestLen = INT_MAX;
				int bestWave = 0;
				List<ID> best_nodes;
				for (auto i = 0; i < temp_wavegroups.size(); ++i) {
					List<ID> tra_nodes;
					dijkstra(temp_wavegroups[i].adjMat, tra, tra_nodes);
					int length = tra_nodes.size();
					if (length != 0 && length <= H && length < bestLen) {
						bestWave = i;
						bestLen = length;
						best_nodes = tra_nodes;
					}
				}
				if (best_nodes.size() != 0) {
					tra->nodes = best_nodes;
					tra->wave = bestWave;
					temp_wavegroups[bestWave].addTra(tra);
				}
				else {
					temp_wavegroups.push_back(WaveGroup(topo.adjMat, trafficNum));
					int newWave = temp_wavegroups.size() - 1;
					tra->nodes.clear();
					dijkstra(temp_wavegroups[newWave].adjMat, tra, tra->nodes);
					tra->wave = newWave;
					temp_wavegroups[newWave].addTra(tra);
				}
			}
			if (temp_wavegroups.size() < WaveNum) {
				waveGroups.clear();
				for (auto i = 0; i < temp_wavegroups.size(); ++i) {
					waveGroups.push_back(temp_wavegroups[i]);
				}
				WaveNum = temp_wavegroups.size();
			}
			
			iter++;
		}
		
		
	}
	struct NodesDelt {
	public:
		List<ID> nodes;
		int delt;
		int wave;
	};
	bool nodesdeltCompare(const NodesDelt & nd1, const NodesDelt &nd2) {
		return nd1.delt < nd2.delt;
	}
	void RWAsolver::bfd_decide_init_solution()
	{
		List<Traffic *> temp_traffics;
		temp_traffics.resize(trafficNum);
		for (auto i = 0; i < trafficNum; ++i) {
			temp_traffics[i] = &traffics[i];
		}
		sort(temp_traffics.begin(), temp_traffics.end(), TrafficPointerCompare);
		int start = 0;
		/*
		//将长度相同的随机达伦
		while (true) {
			int end;
			int i;
			for (i = start; i < temp_traffics.size() - 1; ++i) {
				if (temp_traffics[i]->shorestNodes.size() != temp_traffics[i + 1]->shorestNodes.size()) {
					end = i;
					break;
				}
			}
			shuffleTraffics(temp_traffics, start, end);
			start = end + 1;
			if (i == temp_traffics.size() - 1)break;
		}*/
		int H = max((int)temp_traffics[0]->shorestNodes.size(), (int)sqrt(linkNum));
		List<NodesDelt> nodesdelt;
		waveGroups.clear();
		waveGroups.reserve(WaveNum);
		for (auto w = 0; w < WaveNum; ++w) {
			waveGroups.push_back(WaveGroup(topo.adjMat, trafficNum));
		}
		conflictsNum = 0;
		for (auto i = 0; i < temp_traffics.size(); i++) {
			Traffic *tra = temp_traffics[i];
			int bestLen = INT_MAX;
			int bestWave = 0;
			List<ID> best_nodes;
			for (auto i = 0; i < waveGroups.size(); ++i) {
				List<ID> tra_nodes;
				dijkstra(waveGroups[i].adjMat, tra, tra_nodes);
				int length = tra_nodes.size();
				if (length != 0 && length <= H && length < bestLen) {
					bestWave = i;
					bestLen = length;
					best_nodes = tra_nodes;
				}
			}
			if (best_nodes.size() != 0) {
				tra->nodes = best_nodes;
				tra->wave = bestWave;
				waveGroups[bestWave].addTra(tra);
			}
			else {
				nodesdelt.clear();
				nodesdelt.resize(WaveNum);

				for (auto i = 0; i < waveGroups.size(); ++i) {
					nodesdelt[i].wave = i;
					dijkstraMinConflicts(waveGroups[i].adjMatTraNums, tra, nodesdelt[i].delt, nodesdelt[i].nodes);
				}
				int K = WaveNum/4;// 从冲突增加量前K小的波长中随机选择一个
				partial_sort(nodesdelt.begin(), nodesdelt.begin() + K, nodesdelt.end(), nodesdeltCompare);
				int index = rand() % K;
				tra->nodes = nodesdelt[index].nodes;
				tra->wave = nodesdelt[index].wave;
				bestWave = tra->wave;

				conflictsNum+= waveGroups[bestWave].addTra(tra);
			}
		}
		for (auto w = 0; w < WaveNum; ++w) {
			waveGroups[w].recordConflictTras(topo, conflictTras);
		}
		check_all(1);
	}

	void RWAsolver::bfd_decide_init_solution_withEjectionChain()
	{
		List<Traffic *> temp_traffics;
		temp_traffics.resize(trafficNum);
		for (auto i = 0; i < trafficNum; ++i) {
			temp_traffics[i] = &traffics[i];
		}
		sort(temp_traffics.begin(), temp_traffics.end(), TrafficPointerCompare);
		int start = 0;
		int H = max((int)temp_traffics[0]->shorestNodes.size(), (int)sqrt(linkNum));

		waveGroups.reserve(WaveNum);
		for (auto w = 0; w < WaveNum; ++w) {
			waveGroups.push_back(WaveGroup(topo.adjMat, trafficNum));
		}
		conflictsNum = 0;
		for (auto i = 0; i < temp_traffics.size(); i++) {
			Traffic *tra = temp_traffics[i];
			int bestLen = INT_MAX;
			int bestWave = 0;
			List<ID> best_nodes;
			for (auto i = 0; i < waveGroups.size(); ++i) {
				List<ID> tra_nodes;
				dijkstra(waveGroups[i].adjMat, tra, tra_nodes);
				int length = tra_nodes.size();
				if (length != 0 && length <= H && length < bestLen) {
					bestWave = i;
					bestLen = length;
					best_nodes = tra_nodes;
				}
			}
			if (best_nodes.size() != 0) {
				tra->nodes = best_nodes;
				tra->wave = bestWave;
				waveGroups[bestWave].addTra(tra);
			}
			else {

				int best_delt = INT_MAX;
				int new_Wave = 0;
				int cnt = 1;
				for (auto w = 0; w < WaveNum ; ++w) {
					tempNewNodes.delAllElem();
					int delta = tryAddTraToNewBoxByEjectionChain_Grasp(tra, waveGroups[w], tempNewNodes);
					if (delta < best_delt) {
						new_Wave = w;
						best_newTrasNodes = tempNewNodes;
						best_delt = delta;
						cnt = 1;
					}
					else if (delta == best_delt) {
						cnt++; 
						if (cntCentProbChoice(cnt)) {
							new_Wave = w;
							best_newTrasNodes = tempNewNodes;
						}
					}
				}
				for (auto i = 0; i < best_newTrasNodes.Num; ++i) {
					Traffic *tra = &traffics[best_newTrasNodes.data[i]];
					tra->nodes = tra->best_changeNodes;
				}
				conflictsNum += best_delt;
				waveGroups[new_Wave].addTra(tra);
				tra->wave = new_Wave;
				waveGroups[new_Wave].afreshAdjMat(topo.adjMat);
				waveGroups[new_Wave].recordConflictTras(topo, conflictTras);
			}
		}
		for (auto w = 0; w < WaveNum; ++w) {
			waveGroups[w].recordConflictTras(topo, conflictTras);
		}
	}
	void RWAsolver::check_all(int flag)
	{
#ifdef DEBUG_STATE
		int temp_conflictsNum = 0;
		int conflictTraNums = 0;
		Set<Traffic *> total_conflictTras;
		for (auto w = 0; w < WaveNum; ++w) {
			List<List<ID>> adjMatTrasNum;
			
			WaveGroup &wavegroup = waveGroups[w];
			int group_conflictNum = check_groupConfAndAdjMatNum(wavegroup, adjMatTrasNum, w);
			temp_conflictsNum += group_conflictNum;
			if (group_conflictNum != wavegroup.conflictsOnAllLinks) {
				Log(LogSwitch::FY) << "group "<< w<<" conflictsNum error, actual num is " << temp_conflictsNum << " false =" << conflictsNum << endl;

			}
			Set<Traffic *> groupConflictTras;
			for (auto t = 0; t < wavegroup.tras.size(); ++t) {
				List<ID> &nodes = wavegroup.tras[t]->nodes;
				for (auto j = 0; j < nodes.size() - 1; ++j) {
					ID node1 = nodes[j], node2 = nodes[j + 1];
					if (adjMatTrasNum[node1][node2] > 1) {
						groupConflictTras.insert(wavegroup.tras[t]);
						break;
					}
				}
			}
			conflictTraNums += groupConflictTras.size();
			total_conflictTras.insert(groupConflictTras.begin(),groupConflictTras.end());
			if (wavegroup.groupConflicTras.Num != groupConflictTras.size()) {
				Log(LogSwitch::FY) << "wave " << w << " groupConflictTras size error "
					<<wavegroup.groupConflicTras.Num<<" "<<groupConflictTras.size() << endl;
			}
			Set<Traffic *> tempSet;
			for (auto i = 0; i < wavegroup.groupConflicTras.Num; ++i) {
				tempSet.insert(&traffics[wavegroup.groupConflicTras.data[i]]);
			}
			auto iter1 = tempSet.begin();
			for (auto iter = groupConflictTras.begin(); iter != groupConflictTras.end(); ++iter,++iter1) {
				if (*iter != *iter1) {
					Log(LogSwitch::FY) << "wave " << w << " groupConflictTras error, Traffic:" 
						<<(*iter)->id<<" "<<(*iter1)->id <<  endl;
					//break;
				}
			}
		}
		if (conflictTraNums != conflictTras.Num) {
			Log(LogSwitch::FY) << "total conflict TraNum error, actual num is " << conflictTraNums << " false =" << conflictTras.Num << endl;
			for (auto iter = total_conflictTras.begin(); iter !=total_conflictTras.end(); iter++){
				cout << (*iter)->id << " ";
			}
			cout << endl;
		}
		if (temp_conflictsNum != conflictsNum && flag == 1) {
			Log(LogSwitch::FY) << "total conflictsNum error, actual num is "<<temp_conflictsNum<<" false ="<<conflictsNum<<endl;
		}
#endif // DEBUG_STATE

	}

	int RWAsolver::check_groupConfAndAdjMatNum(WaveGroup & wavegroup, List<List<ID>> &adjMatTrasNum,int w)
	{
		adjMatTrasNum.resize(nodeNum);
		for (auto i = 0; i < nodeNum; ++i) {
			adjMatTrasNum[i].resize(nodeNum, 0);
		}
		int groupConflictNum = getGroupConflictNum(wavegroup, adjMatTrasNum);
		if (groupConflictNum != wavegroup.conflictsOnAllLinks) {
			Log(LogSwitch::FY) << "wave " << w << " realconf ="<<groupConflictNum<<" false = "<<wavegroup.conflictsOnAllLinks<<endl;
		}

		for (auto i = 0; i < nodeNum; ++i) {
			for (auto j = 0; j < nodeNum; ++j) {
				if (adjMatTrasNum[i][j] != wavegroup.adjMatTraNums[i][j]) {
					Log(LogSwitch::FY) << "wave " << w << " adjMatTrasNum error\n";
				}
			}
		}
		
		
		return groupConflictNum;
	}

	int RWAsolver::getGroupConflictNum(WaveGroup & wavegroup, List<List<ID>> &adjMatTrasNum)
	{
		int groupConflictNum = 0;
		List<Traffic*> &s = wavegroup.tras;
		for (auto iter = s.begin(); iter != s.end(); ++iter) {
			List<ID> &nodes = (*iter)->nodes;
			for (auto j = 0; j < nodes.size() - 1; ++j) {
				ID node1 = nodes[j], node2 = nodes[j + 1];
				adjMatTrasNum[node1][node2]++;
				if (adjMatTrasNum[node1][node2] > 1) {
					groupConflictNum++;
				}
			}
		}
		return groupConflictNum;
	}


	void RWAsolver::findConflicts()
	{
//		double start = timer.elapsedSeconds();
//
//		conflictsNum = 0;
//		int cnt=0;
//		for (auto original_w = 0; original_w < WaveNum; ++original_w) {
//			if (waveGroups[original_w].conflictsOnAllLinks != 0) {
//				cnt++;
//				Model model(waveGroups[original_w].tras, nodeNum, topo, grb_env,Interger); //使用模型最小化同一个波长组的业务的冲突
//
//				model.getAnswer(waveGroups[original_w].tras, topo);
//				waveGroups[original_w].conflictsOnAllLinks = model.traNumShouldRemove;
//				cout << model.traNumShouldRemove<<" wave = "<<original_w<<", trasNum = "<<waveGroups[original_w].tras.size()<<endl;
//				conflictsNum += model.traNumShouldRemove;
//				
//				waveGroups[original_w].afreshAdjMat(topo.adjMat);
//
//				//waveGroups[original_w].printTras();
//				waveGroups[original_w].recordConflictTras(topo);
//
//#ifdef DEBUG_FY
//				int temp_cnt = 0;
//				for (auto i = 0; i < nodeNum; ++i) {
//					for (auto j = 0; j < nodeNum; ++j) {
//						if (waveGroups[original_w].adjMatTraNums[i][j] > 1) {
//							temp_cnt += (waveGroups[original_w].adjMatTraNums[i][j] - 1);
//						}
//					}
//				}
//				if (temp_cnt != waveGroups[original_w].conflictsOnAllLinks) {
//					Log(LogSwitch::FY) << "obj error " << temp_cnt << " "<< waveGroups[original_w].conflictsOnAllLinks << endl;
//
//					conflictsNum = conflictsNum - waveGroups[original_w].conflictsOnAllLinks + temp_cnt;
//					waveGroups[original_w].conflictsOnAllLinks = temp_cnt;
//				}
//#endif // DEBUG_FY
//
//				
//			}
//		}
//		best_conflictsNum = conflictsNum;
//		double avg = ((timer.elapsedSeconds() - start) / cnt);
//		cout <<avg<< endl;

	}

	int RWAsolver::findEjectionMove(WaveGroup &wavegroup, Traffic * addedTra, EjectiveMove & ecmove)
	{
		if (wavegroup.groupConflicTras.Num == 0) {
			if (wavegroup.conflictsOnAllLinks != 0) {
				Log(LogSwitch::FY) << "groupConflicTras equals 0 while conflictsOnAllLinks not equals 0" << endl;
			}
			return 0;
		}
		Traffic * pickTra=NULL;
		int best_delt = 0;

		// 挑选出一条冲突数最多的业务
		//pickTra = pickMostConflictTraffic(wavegroup, addedTra);
		// 随机挑选一条业务拿起来
		pickTra = pickRandomConflictTraffic(wavegroup, addedTra);

		List<ID> oldPickNodes = pickTra->nodes;
		int pick_oldDelt = wavegroup.deleteTra(pickTra, conflictTras); // 将pickTra的资源释放
		wavegroup.recordConflictTras(topo, conflictTras);
		int cnt = 1;
		List<ID> &groupConfTras = wavegroup.groupConflicTras.data;
		int groupConfNum = wavegroup.groupConflicTras.Num;
		for (auto i = 0; i < groupConfNum; ++i) {
			Traffic *tra = &traffics[groupConfTras[i]];

			if (tra == pickTra) continue;
			List<ID> oldrrTraNodes = tra->nodes;
			int old_conflictNum = 0, new_conflictNum = 0;
			old_conflictNum = wavegroup.deleteTraWithoutChangeConflict(tra); // 准备重新路由，先释放自己占用的资源
			List<ID> reroutingNodes;
			dijkstraMinConflicts(wavegroup.adjMatTraNums, tra, new_conflictNum,reroutingNodes);
			tra->nodes = reroutingNodes;
			wavegroup.addTra(tra);
			int pick_newDelt = 0;
			List<ID> newPickNodes;
			dijkstraMinConflicts(wavegroup.adjMatTraNums, pickTra, pick_newDelt,newPickNodes);
			int temp_delt = pick_oldDelt + old_conflictNum - (pick_newDelt + new_conflictNum);
			if ( temp_delt > best_delt) { // 说明有改进
				best_delt = temp_delt;
				ecmove.delt = best_delt;
				ecmove.pickNewNodes = newPickNodes;
				ecmove.pickTra = pickTra;
				ecmove.reroutingTra = tra;
				ecmove.reroutingNodes = reroutingNodes;
				cnt = 1;
			}
			else if (temp_delt == best_delt) {
				cnt++;
				if (cntCentProbChoice(cnt)) {
					ecmove.delt = best_delt;
					ecmove.pickNewNodes = newPickNodes;
					ecmove.pickTra = pickTra;
					ecmove.reroutingTra = tra;
					ecmove.reroutingNodes = reroutingNodes;
				}
			}
			// 恢复原有的路径以及网络环境
			wavegroup.deleteTraWithoutChangeConflict(tra);
			tra->nodes = oldrrTraNodes;
			wavegroup.addTra(tra);
		}
		wavegroup.addTra(pickTra);
		wavegroup.recordConflictTras(topo, conflictTras);
		return best_delt;
	}

	int RWAsolver::EjectionChain(WaveGroup & wavegroup, Traffic * addedTra)
	{
		return 0;
	}
	void getKnumber(int k, int n, List<ID> &nums) {
		nums.resize(n);
		for (auto i = 0; i < n; ++i) {
			nums[i] = i;
		}
		if (k < n) {
			for (auto i = 0; i < k; ++i) {
				int index = rand() % (n - i) + i;
				int tmp = nums[i];
				nums[i] = nums[index];
				nums[index] = tmp;
			}
		}
	}
	
	int RWAsolver::findBestRerouting(WaveGroup &wavegroup, TwoWayList& groupConflictTras, ReroutingMove &rrmove)
	{
		int best_delt = -1, randSel_delt=-1;
		int cnt = 1,randSelCnt=1;
		Traffic *best_tra = NULL,*randSel_tra =NULL;
		List<ID> best_nodes, randSelNodes;

		List<ID> &groupConfTras = groupConflictTras.data;
		int groupConfNum = groupConflictTras.Num;
		//int evaluateNum = groupConfNum;
		int evaluateNum = min(3, groupConfNum);
		List <ID> evaluateIndexs;
		getKnumber(evaluateNum, groupConfNum, evaluateIndexs);
		for (auto i = 0; i < evaluateNum; ++i) {
			//int index = i;
			int index = evaluateIndexs[i];
			Traffic *tra = &traffics[groupConfTras[index]];

			List<ID> new_nodes;
			int temp_delt = calculateIncomeForRerouting(wavegroup.adjMatTraNums, tra, new_nodes);
			if (temp_delt < 0) {
				Log(LogSwitch::FY) << "Rerouting move error\n";
				exit(-4);
			}
			if (temp_delt > best_delt) {
				best_delt = temp_delt;
				best_tra = tra;
				best_nodes = new_nodes;
				cnt = 1;
			}
			else if (temp_delt == best_delt) {
				cnt++;
				if (cntCentProbChoice(cnt)) {
					best_delt = temp_delt;
					best_tra = tra;
					best_nodes = new_nodes;
				}
			}

			if (cntCentProbChoice(randSelCnt)) {
				randSel_tra = tra;
				randSelNodes = new_nodes;
				randSel_delt = temp_delt;
			}
			randSelCnt++;
		}
		if (cntCentProbChoice(8)) { // 以1/5的概率随机选择一个
			rrmove.delt = randSel_delt;
			rrmove.new_nodes = randSelNodes;
			rrmove.tra = randSel_tra;
		}
		else {
			rrmove.delt = best_delt;
			rrmove.tra = best_tra;
			rrmove.new_nodes = best_nodes;
		}
	}

	int RWAsolver::calculateIncomeForRerouting(List<List<ID>>& adjMatTraNums, Traffic * tra, List<ID> &new_nodes)
	{
		int old_conflictNum = 0, new_conflictNum = 0;

		for (auto j = 0; j < tra->nodes.size() - 1; ++j) {// 将tra占用的资源释放
			ID node1 = tra->nodes[j], node2 = tra->nodes[j + 1];
			if (adjMatTraNums[node1][node2] > 1) {// 说明存在冲突
				old_conflictNum++;
			}
			if (adjMatTraNums[node1][node2] == 0) {
				Log(LogSwitch::FY) << "calculateIncomeForRerouting chain error\n";
				exit(-4);
			}
			adjMatTraNums[node1][node2]--;
		}
		dijkstraMinConflicts(adjMatTraNums, tra, new_conflictNum, new_nodes);

		for (auto j = 0; j < tra->nodes.size() - 1; ++j) {// tra重新占用资源
			ID node1 = tra->nodes[j], node2 = tra->nodes[j + 1];
			adjMatTraNums[node1][node2]++;
		}
		int temp_delt = old_conflictNum - new_conflictNum; // temp_delt >= 0 
		return temp_delt;
	}


	struct Dnode {/*用于dijkstra的node*/
		ID id;
		int dist;
		Dnode(ID id, int dist) :id(id), dist(dist) {}
		bool operator<(const Dnode k)const
		{
			return dist > k.dist;
		}
	};
	struct Ddnode {/*用于dijkstra的node*/
		ID id;
		double dist;
		Ddnode(ID id, double dist) :id(id), dist(dist) {}
		bool operator<(const Ddnode k)const
		{
			return dist > k.dist;
		}
	};
	struct DCnode {/*用于dijkstraMinConflicts的node*/
		ID id;
		int obj1;
		int obj2;
		DCnode(ID id, int obj1,int obj2) :id(id), obj1(obj1),obj2(obj2) {}
		bool operator<(const DCnode k)const
		{
			if (obj1 > k.obj1) return true;
			else if (obj1 == k.obj1) {
				return obj2 > k.obj2;
			}
			else
				return false;
		}
	};

	//void RWAsolver::dijkstraMinConflicts(List<List<int>>& adjMatTraNums, Traffic * tra, int &conflictsNum, List<ID> &path)
	//{
	//	priority_queue<Dnode> q;
	//	List<int> dist;
	//	List<ID> prev;
	//	dist.resize(nodeNum);
	//	prev.resize(nodeNum);
	//	for (int i = 0; i < nodeNum; ++i) {
	//		dist[i] = MaxDistance;
	//		prev[i] = InvalidID;
	//	}
	//	ID src = tra->src, dst = tra->dst;
	//	dist[src] = 0;
	//	q.push(Dnode(src, 0));
	//	while (!q.empty()) {
	//		Dnode n = q.top();
	//		q.pop();
	//		ID v = n.id;
	//		if (dst == v) break;
	//		int link_size = topo.linkNodes[v].size();
	//		for (auto i = 0; i < link_size; ++i) {
	//			ID to_id = topo.linkNodes[v][i];
	//			int alt = n.dist;
	//			if (adjMatTraNums[v][to_id] > 0) {// 说明该边已经被其它业务占用
	//				alt++;
	//			}
	//			if (dist[to_id] > alt) {
	//				dist[to_id] = alt;
	//				q.push(Dnode(to_id, alt));
	//				prev[to_id] = v;
	//			}
	//		}
	//	}
	//	ID v = dst;
	//	path.clear();
	//	path.reserve(20);
	//	conflictsNum = dist[dst];
	//	while (prev[v] != InvalidID)
	//	{
	//		path.push_back(v);
	//		v = prev[v];
	//	}
	//	if (path.size() > 0) {
	//		path.push_back(src);
	//	}
	//	reverse(path.begin(), path.end());
	//}
	//
	
	
	void RWAsolver::dijkstraMinConflicts(List<List<int>>& adjMatTraNums, Traffic * tra,int &conflictsNum,List<ID> &path)
	{
		dij_cnt++;
		priority_queue<DCnode> q;
		List<int> obj1;
		List<int> obj2;
		List<ID> prev;
		obj1.resize(nodeNum,MaxDistance);
		obj2.resize(nodeNum,MaxDistance);
		prev.resize(nodeNum,InvalidID);
		/*for (int i = 0; i < nodeNum; ++i) {
			obj1[i] = MaxDistance;
			obj2[i] = MaxDistance;
			prev[i] = InvalidID;
		}*/
		ID src = tra->src, dst = tra->dst;
		obj1[src] = 0;
		obj2[src] = 0;
		q.push(DCnode(src, 0,0));
		while (!q.empty()) {
			DCnode n = q.top();
			q.pop();
			ID v = n.id;

			if (dst == v) break;
			List<ID> &linkNodes_v = topo.linkNodes[v];
			int link_size = linkNodes_v.size();
			for (auto i = 0; i < link_size; ++i) {
				ID to_id = linkNodes_v[i];
				int alt1 = n.obj1;
				int alt2 = n.obj2;
				if (adjMatTraNums[v][to_id] > 0) {// 说明该边已经被其它业务占用
					alt1++;
				}
				alt2++;
				if (obj1[to_id] > alt1) {
					obj1[to_id] = alt1;
					obj2[to_id] = alt2;
					q.push(DCnode(to_id, alt1,alt2));
					prev[to_id] = v;
				}
				else if (obj1[to_id] == alt1) {
					if (obj2[to_id] > alt2) {
						obj2[to_id] = alt2;
						q.push(DCnode(to_id, alt1, alt2));
						prev[to_id] = v;
					}
				}
			}
		}

		ID v = dst;
		path.clear();
		path.reserve(20);
		conflictsNum = obj1[dst];
		while (prev[v] != InvalidID)
		{
			path.push_back(v);
			v = prev[v];
		}
		if (path.size() > 0) {
			path.push_back(src);
		}
		reverse(path.begin(), path.end());
	}

	void RWAsolver::dijkstra(List<List<int>>& adjMat, Traffic * tra, List<ID> &path)
	{
		
		priority_queue<Dnode> q;
		List<int> dist;
		List<ID> prev;
		dist.resize(nodeNum, MaxDistance);
		prev.resize(nodeNum, InvalidID);
		/*for (int i = 0; i < nodeNum; ++i) {
			dist[i] = MaxDistance;
			prev[i] = InvalidID;
		}*/
		ID src = tra->src,dst = tra->dst;
		dist[src] = 0;
		q.push(Dnode(src, 0));

		while (!q.empty()){
			Dnode n = q.top();
			q.pop();
			ID v = n.id;

			if (dst == v) break;
			int link_size = topo.linkNodes[v].size();
			for (auto i = 0; i < link_size; ++i) {
				ID to_id = topo.linkNodes[v][i];
				if (adjMat[v][to_id] == MaxDistance) continue;
				int alt = n.dist + adjMat[v][to_id];
				if (dist[to_id] > alt) {
					dist[to_id] = alt;
					q.push(Dnode(to_id, alt));
					prev[to_id] = v;
				}
			}
		}

		ID v = dst;
		path.clear();
		path.reserve(20);
		while (prev[v] != InvalidID)
		{
			path.push_back(v);
			v = prev[v];
		}
		if (path.size() > 0){
			path.push_back(src);
		}
		reverse(path.begin(), path.end());
	}

	void RWAsolver::deployTrasByGraphColoring()
	{
		/*List<Traffic *> all_traffics;
		for (auto t = 0; t < trafficNum; ++t) {
			all_traffics.push_back(&traffics[t]);
		}
		Model model(all_traffics,nodeNum,topo,grb_env,Interger,MultiCommodity,WaveNum);
		model.getAnswer(all_traffics, topo);

		gc_aux.nodes.resize(trafficNum);
		for (auto i = 0; i < trafficNum; ++i) {
			gc_aux.nodes[i] = i;
		}
		for (auto i = 0; i < traffics.size(); ++i) {
			Traffic *tra1 = &traffics[i];
			for (auto j = i+1; j < traffics.size(); ++j) {
				Traffic *tra2 = &traffics[j];
				bool overlap = false;
				for (auto n1 = 0; (n1 < tra1->nodes.size()-1) && !overlap; ++n1) {
					ID node1 = tra1->nodes[n1], node2 = tra1->nodes[n1 + 1];
					for (auto n2 = 0; n2 < tra2->nodes.size()-1; ++n2) {
						ID node3 = tra2->nodes[n2], node4 = tra2->nodes[n2 + 1];
						if ((node1 == node3 && node2 == node4)) {
							overlap = true;
							break;
						}
					}
				}
				if (overlap) {
					gc_aux.edges.push_back(make_pair(i, j));
				}
			}
		}
		List<ID> WaveLens;
		cout << "Nodes:" << gc_aux.nodes.size() << " Edges:" << gc_aux.edges.size() << endl;
		WaveLens = graph_coloring(gc_aux.nodes, gc_aux.edges,WaveNum);
		for (auto i = 0; i < WaveLens.size(); ++i) {
			auto w = WaveLens[i];
			traffics[i].nodes = traffics[i].shorestNodes;
			waveGroups[w].addTra(&traffics[i]);
			traffics[i].wave = w;
		}
		conflictsNum = 0;
		for (auto w = 0; w < WaveNum; ++w) {
			waveGroups[w].afreshAdjMat(topo.adjMat);
			waveGroups[w].recordConflictTras(topo, conflictTras);
			conflictsNum += waveGroups[w].conflictsOnAllLinks;
		}*/
		
	}

	void RWAsolver::greedyDeployTras()
	{
		waveGroups.reserve(WaveNum);
		for (auto w = 0; w < WaveNum; ++w) {
			WaveGroup waveGroup(topo.adjMat,trafficNum);
			waveGroups.push_back(waveGroup);
		}
		conflictsNum = 0;
		for (auto i = 0; i < traffics.size(); ++i) {
			bool find = false;
			for (auto w = 0; w < WaveNum; ++w) { // 遍历所有颜色盒子
				dijkstra(waveGroups[w].adjMat, &traffics[i], traffics[i].nodes);
				if (traffics[i].nodes.size() != 0) {// 说明在该颜色盒子中能找到无冲突的路径
					find = true;
					waveGroups[w].addTra(&traffics[i]);
					traffics[i].wave = w;
					break;
				}
			}
			if (!find) {// 随机分配一种颜色
				int w = rand() % WaveNum;
				//traffics[i].nodes = traffics[i].shorestNodes; 
				int con;
				dijkstraMinConflicts(waveGroups[w].adjMatTraNums, &traffics[i], con, traffics[i].nodes);
				int res = waveGroups[w].addTra(&traffics[i]);
				conflictsNum += res;
				traffics[i].wave = w;
			}
		}
		for (auto w = 0; w < WaveNum; ++w) {
			waveGroups[w].afreshAdjMat(topo.adjMat);
			waveGroups[w].recordConflictTras(topo, conflictTras);
		}
		check_all();
	}

	void RWAsolver::subColor(List<WaveGroup> &temp_waveGroups) {
		int minWave = 0; // 包含业务数最小的波长
		int min_size = INT_MAX;
		for (auto i = 0; i < WaveNum + 1; ++i) {
			if (temp_waveGroups[i].tras.size() < min_size) {
				minWave = i;
				min_size = temp_waveGroups[i].tras.size();
			}
		}
		// 将minWave中的业务贪心分配到其它颜色盒子中

		List<Traffic *> &s = temp_waveGroups[minWave].tras;
		sort(s.begin(), s.end(), TrafficPointerCompare);
		for (auto iter = s.begin(); iter != s.end(); ++iter) {
			Traffic *tra = *iter;

			int conflictNum = INT_MAX;
			int new_Wave = 0;
			int cnt = 1;
			for (auto w = 0; w < WaveNum + 1; ++w) {
				if (w != minWave) {
					tempNewNodes.delAllElem();
					int delta = tryAddTraToNewBoxByEjectionChain_Grasp(tra, temp_waveGroups[w], tempNewNodes);
					if (delta < conflictNum) {
						new_Wave = w;
						best_newTrasNodes = tempNewNodes;
						conflictNum = delta;
						cnt = 1;
					}
					else if (delta == conflictNum ) {
						cnt++; if (cntCentProbChoice(cnt)) {
							new_Wave = w;
							best_newTrasNodes = tempNewNodes;
						}
					}
				}
			}
			for (auto i = 0; i < best_newTrasNodes.Num; ++i) {
				Traffic *tra = &traffics[best_newTrasNodes.data[i]];
				tra->nodes = tra->best_changeNodes;
			}
			temp_waveGroups[new_Wave].addTra(tra);
			tra->wave = new_Wave;
			temp_waveGroups[new_Wave].afreshAdjMat(topo.adjMat);
			temp_waveGroups[new_Wave].recordConflictTras(topo, conflictTras);
			/*temp_waveGroups[new_Wave].addTra(tra);
			temp_waveGroups[new_Wave].recordConflictTras(topo, conflictTras);*/
		}
		cout << conflictTras.Num << endl;
		waveGroups.clear();
		for (auto w = 0; w < WaveNum + 1; ++w) {
			if (w != minWave) {
				WaveGroup wg(temp_waveGroups[w]);
				List<Traffic *> &s = temp_waveGroups[w].tras;
				for (auto iter = s.begin(); iter != s.end(); ++iter) {
					(*iter)->wave = waveGroups.size();
				}
				waveGroups.push_back(wg);
				conflictsNum += wg.conflictsOnAllLinks;
			}
		}
		check_all();
	}
	void RWAsolver::deployByFileAndSubColor()
	{
		// 文件中使用的颜色数需要恰好比WaveNum多1
		string Filename("box_results\\");
		Filename = Filename + env.insName + "_result"+to_string(WaveNum+1)+".txt";
		List<WaveGroup> temp_waveGroups;
		temp_waveGroups.reserve(WaveNum+1);
		for (auto w = 0; w < WaveNum+1; ++w) {
			WaveGroup waveGroup(topo.adjMat,trafficNum);
			temp_waveGroups.push_back(waveGroup);
		}

		ifstream fin(Filename.c_str());
		if (fin.fail() || fin.eof())
		{
			cout << "### Erreur open, File_Name " << Filename << endl;
			exit(0);
		}
		string headline;
		getline(fin, headline);
		getline(fin, headline);

		int color, lp;
		int cnt = 0;
		while (!fin.eof())
		{
			fin >> lp >> color;
			if (color > WaveNum) {
				cout << "the color num is too big\n" << endl;
				exit(-3);
			}
			traffics[lp].wave = color;
			
			
			int temp;
			fin >> temp;
			for (auto i = 0; i < temp; ++i) {
				int c;
				fin >> c;
				traffics[lp].nodes.push_back(c);
			}
			temp_waveGroups[color].addTra(&traffics[lp]);
			if (temp_waveGroups[color].conflictsOnAllLinks > 0) {
				cout << "the conflict num is not zero\n";
				exit(-4);
			}
		}
		cout << conflictTras.Num << endl;
		//int same_cnt=0;
		//for (auto i = 0; i < WaveNum+1; ++i) {
		//	cout << temp_waveGroups[i].tras.size()<<" ---------------------\n\n";
		//	for (auto j = 0; j < temp_waveGroups[i].tras.size(); ++j) {
		//		if (temp_waveGroups[i].tras[j]->nodes.size() <= temp_waveGroups[i].tras[j]->shorestNodes.size()+2)
		//			same_cnt++;
		//		//cout << temp_waveGroups[i].tras[j]->nodes.size()<<"  "<< temp_waveGroups[i].tras[j]->shorestNodes.size()<<endl;
		//	}
		//	cout << endl;
		//}
		//cout << same_cnt << endl;
		subColor(temp_waveGroups);
	}

	void RWAsolver::deployByMemoryAndSubColor()
	{
		List<WaveGroup> temp_waveGroups;
		for (auto w = 0; w < WaveNum + 1; ++w) {
			temp_waveGroups.push_back(waveGroups[w]);
		}
		subColor(temp_waveGroups);
	}

	void addChangedTrasNodes(Map<Traffic*,List<ID>> &newTrasNodes, Traffic *tra) {
		auto iter = newTrasNodes.find(tra);
		if (iter != newTrasNodes.end()) {// 说明Map中已经有tra
			(*iter).second = tra->nodes;
		}
		else {
			newTrasNodes.insert(make_pair(tra, tra->nodes));
		}
	}

	Traffic* RWAsolver::getOneConflictTra() {
		/*int temp = rand() % (confWaveGroups.Num);
		int confwave = confWaveGroups.data[temp];
		int index = rand() % waveGroups[confwave].groupConflicTras.size();
		Set<Traffic *>::iterator iter;
		iter = select_random(waveGroups[confwave].groupConflicTras, index);
		return *iter;*/
		int conflictTraNum = conflictTras.Num;
		if (conflictsNum == 0) return NULL;
		int index;
		Set<Traffic *>::iterator iter;
		index = rand() % conflictTraNum;//从有冲突的业务中随机选择一个
		
		return &traffics[conflictTras.data[index]];
	}
	

	//int RWAsolver::tryAddTraToNewBoxByEjectionChain_Grasp(Traffic * conflictTra, WaveGroup &wavegroup, TwoWayList& addTraNewTrasNodes)
	//{
	//	int add_delt = 0, best_add_delt, current_add_delt;
	//	// 评估conflictTra放入new_w这个颜色盒子增加的冲突数
	//	List<List<ID>> adjMatNums;
	//	dijkstraMinConflicts(wavegroup.adjMatTraNums, conflictTra, add_delt, conflictTra->nodes); // add_delt是增加的冲突数
	//	currentNewTrasNodes.delAllElem();
	//	current_add_delt = best_add_delt = add_delt;
	//	currentNewTrasNodes.insertElem(conflictTra->id);
	//	conflictTra->best_changeNodes = conflictTra->nodes;
	//	if (add_delt > 0) { //需要通过EjectionChain降低冲突
	//		for (auto i = 0; i < currentNewTrasNodes.Num; ++i) {
	//			addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
	//		}
	//		wavegroup.addTra(conflictTra);// conflictTra放入盒子中,得到当前最优解
	//		wavegroup.recordConflictTrasNotChangeGlobal(topo); // 更新冲突业务
	//		int nobetterTimes = 0;
	//		while (true) {
	//			Traffic *pickTra;
	//			if (cntCentProbChoice(4)) {
	//				pickTra = pickRandomConflictTraffic(wavegroup, conflictTra);
	//			}
	//			else {
	//				pickTra = pickMostConflictTraffic(wavegroup, conflictTra);
	//			}
	//			if (pickTra == NULL) break;
	//			int pick_oldDelt = wavegroup.deleteTraNotChangeGlobal(pickTra); // 将pickTra的资源释放,得到部分解
	//			int chain_delt = 0; // 该ejectionChain的改进程度
	//			int best_delt = 0, cnt = 1, randSelCnt = 1, randSel_delt = 0;
	//			Traffic *best_tra = NULL, *randSel_tra = NULL;
	//			List<ID> bestNodes, randSelNodes;
	//			List<ID> &groupConfTras = wavegroup.groupConflicTras.data;
	//			int groupConfNum, evaluateNum, groupTraNum;
	//			groupConfNum = wavegroup.groupConflicTras.Num;
	//			if (groupConfNum == 0) {
	//				wavegroup.addTra(pickTra);
	//				cout << "testffff\n";
	//				break;
	//			}
	//			evaluateNum = min(3, groupConfNum);
	//			List <ID> evaluateIndexs;
	//			getKnumber(evaluateNum, groupConfNum, evaluateIndexs);
	//			for (auto i = 0; i < evaluateNum; ++i) {
	//				Traffic *tra;
	//				int index = evaluateIndexs[i];
	//				//int index = i;
	//				tra = &traffics[groupConfTras[index]];
	//				if (tra == pickTra) continue;
	//				int old_conflictNum = 0, new_conflictNum = 0;
	//				List<ID> reroutingNodes;
	//				int temp_delt = calculateIncomeForRerouting(wavegroup.adjMatTraNums, tra, reroutingNodes);
	//				if (temp_delt > best_delt) {// 说明部分解(pickTra未放下)
	//					best_delt = temp_delt;
	//					best_tra = tra;
	//					bestNodes = reroutingNodes;
	//					cnt = 1;
	//				}
	//				else if (temp_delt == best_delt) {
	//					cnt++;
	//					if (cntCentProbChoice(cnt)) {
	//						best_delt = temp_delt;
	//						best_tra = tra;
	//						bestNodes = reroutingNodes;
	//					}
	//				}
	//				if (cntCentProbChoice(randSelCnt)) {
	//					randSel_tra = tra;
	//					randSelNodes = reroutingNodes;
	//					randSel_delt = temp_delt;
	//				}
	//				randSelCnt++;
	//			}
	//			if (cntCentProbChoice(6)) { // 以较小的概率随机选择一个
	//				best_delt = randSel_delt;
	//				bestNodes = randSelNodes;
	//				best_tra = randSel_tra;
	//			}
	//			if (best_tra == NULL) {
	//				break;
	//			}
	//			if (!currentNewTrasNodes.isElemExisit(best_tra->id))
	//			{
	//				currentNewTrasNodes.insertElem(best_tra->id);
	//				best_tra->oldNodes = best_tra->nodes;
	//			}
	//			wavegroup.delTraNodes(best_tra);
	//			best_tra->nodes = bestNodes;
	//			wavegroup.addTraNodes(best_tra);
	//			chain_delt += best_delt; // 改变best_tra得到的冲突数的减少量
	//			// 判断是否能改进当前最优解
	//			int putback_delt = 0, temp_add_delt;
	//			if (!currentNewTrasNodes.isElemExisit(pickTra->id))
	//			{
	//				currentNewTrasNodes.insertElem(pickTra->id);
	//				pickTra->oldNodes = pickTra->nodes;
	//			}
	//			dijkstraMinConflicts(wavegroup.adjMatTraNums, pickTra, putback_delt, pickTra->nodes); // 将pickTra放下，获得完整解
	//			wavegroup.addTra(pickTra);
	//			wavegroup.recordConflictTrasNotChangeGlobal(topo);
	//			temp_add_delt = current_add_delt - pick_oldDelt - chain_delt + putback_delt;
	//			if (temp_add_delt < best_add_delt) {
	//				best_add_delt = temp_add_delt;
	//				for (auto i = addTraNewTrasNodes.Num; i < currentNewTrasNodes.Num; ++i) {
	//					addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
	//				}
	//				for (auto i = 0; i < addTraNewTrasNodes.Num; ++i) {
	//					Traffic *t = &traffics[addTraNewTrasNodes.data[i]];
	//					t->best_changeNodes = t->nodes;
	//				}
	//				nobetterTimes = 0;
	//			}
	//			else if (temp_add_delt >= best_add_delt) {
	//				if (cntCentProbChoice(4) && temp_add_delt == best_add_delt) {
	//					for (auto i = addTraNewTrasNodes.Num; i < currentNewTrasNodes.Num; ++i) {
	//						addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
	//					}
	//					for (auto i = 0; i < addTraNewTrasNodes.Num; ++i) {
	//						Traffic *t = &traffics[addTraNewTrasNodes.data[i]];
	//						t->best_changeNodes = t->nodes;
	//					}
	//				}
	//				nobetterTimes++;
	//			}
	//			if (nobetterTimes >= 3) break;
	//		}
	//		wavegroup.deleteTraNotChangeGlobal(conflictTra);
	//		wavegroup.recoverFromOldNodes(traffics, currentNewTrasNodes, conflictTra);
	//		wavegroup.recordConflictTrasNotChangeGlobal(topo);//重新记录冲突
	//	}
	//	else {
	//		//addTraNewTrasNodes = currentNewTrasNodes;
	//		addTraNewTrasNodes.delAllElem();
	//		for (auto i = 0; i < currentNewTrasNodes.Num; ++i) {
	//			addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
	//		}
	//		best_add_delt = 0;
	//	}
	//	return best_add_delt;
	//}

	int RWAsolver::tryAddTraToNewBoxByEjectionChain_Grasp(Traffic * conflictTra, WaveGroup &wavegroup, TwoWayList& addTraNewTrasNodes,bool flag)
	{
		int add_delt = 0, best_add_delt, current_add_delt;
		// 评估conflictTra放入new_w这个颜色盒子增加的冲突数
		List<List<ID>> adjMatNums;
		dijkstraMinConflicts(wavegroup.adjMatTraNums, conflictTra, add_delt, conflictTra->nodes); // add_delt是增加的冲突数
		if(flag) greedy_deltNodes.push_back(make_pair(add_delt,conflictTra->nodes));
		currentNewTrasNodes.delAllElem();
		//evaluatedTras.delAllElem();
		addTraNewTrasNodes.delAllElem();
		current_add_delt = best_add_delt = add_delt;
		currentNewTrasNodes.insertElem(conflictTra->id);
		conflictTra->best_changeNodes = conflictTra->nodes;
		if (add_delt > 0) { //需要通过EjectionChain降低冲突
			//addTraNewTrasNodes = currentNewTrasNodes;
			for (auto i = 0; i < currentNewTrasNodes.Num; ++i) {
				addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
			}
			//wavegroup.recordOldNodes();  // 记录原始解
			wavegroup.addTra(conflictTra);// conflictTra放入盒子中,得到当前最优解

			wavegroup.recordConflictTrasNotChangeGlobal(topo); // 更新冲突业务
			Traffic *pickTra;
			if (cntCentProbChoice(4)) {
				pickTra = pickRandomConflictTraffic(wavegroup, conflictTra);
			}
			else {
				pickTra = pickMostConflictTraffic(wavegroup, conflictTra);
			}
			int pick_oldDelt = wavegroup.deleteTraNotChangeGlobal(pickTra); // 将pickTra的资源释放,得到部分解

			int len = 0;
			int chain_delt = 0; // 该ejectionChain的改进程度
			int nobetterTimes =0 ;
			Traffic *prevEvaluatedTra = NULL;
			while (true) {
				int best_delt = 0, cnt = 1, randSelCnt = 1, randSel_delt = 0;
				Traffic *best_tra = NULL, *randSel_tra = NULL;
				List<ID> bestNodes, randSelNodes;
				List<ID> &groupConfTras = wavegroup.groupConflicTras.data;
				int groupConfNum, evaluateNum,groupTraNum;
				groupConfNum = wavegroup.groupConflicTras.Num;
				List <ID> evaluateIndexs;
				evaluateNum = min(3, groupConfNum);
				if (groupConfNum == 0) {
					break;
				}{
					getKnumber(evaluateNum, groupConfNum, evaluateIndexs);
				}
				for (auto i = 0; i < evaluateNum; ++i) {
					Traffic *tra;
					int index = evaluateIndexs[i];
					//int index = i; (groupConfNum == 0 && conflictsNum <= 3)?wavegroup.tras[index]:
					tra = &traffics[groupConfTras[index]];
					
					if (tra == pickTra 
						//||tra == prevEvaluatedTra
						) continue;
					int old_conflictNum = 0, new_conflictNum = 0;
					List<ID> reroutingNodes;
					int temp_delt = calculateIncomeForRerouting(wavegroup.adjMatTraNums, tra, reroutingNodes);
					if (temp_delt > best_delt) {// 说明部分解(pickTra未放下)
						best_delt = temp_delt;
						best_tra = tra;
						bestNodes = reroutingNodes;
						cnt = 1;
					}
					else if (temp_delt == best_delt) {
						cnt++;
						if (cntCentProbChoice(cnt)) {
							best_delt = temp_delt;
							best_tra = tra;
							bestNodes = reroutingNodes;
						}
					}
					if (cntCentProbChoice(randSelCnt)) {
						randSel_tra = tra;
						randSelNodes = reroutingNodes;
						randSel_delt = temp_delt;
					}
					randSelCnt++;
				}
				if (cntCentProbChoice(6)) { // 以较小的概率随机选择一个
					best_delt = randSel_delt;
					bestNodes = randSelNodes;
					best_tra = randSel_tra;
				}
				if (best_tra == NULL) {
					break;
				}
				if (!currentNewTrasNodes.isElemExisit(best_tra->id))
				{
					currentNewTrasNodes.insertElem(best_tra->id);
					best_tra->oldNodes = best_tra->nodes;
				}
				wavegroup.delTraNodes(best_tra);
				best_tra->nodes = bestNodes;
				prevEvaluatedTra = best_tra;
				//evaluatedTras.insertElem(best_tra->id);
				wavegroup.addTraNodes(best_tra);
				wavegroup.recordConflictTrasNotChangeGlobal(topo);

				chain_delt += best_delt; // 改变best_tra得到的冲突数的减少量

				// 判断是否能改进当前最优解
				int putback_delt = 0, temp_add_delt;
				if (!currentNewTrasNodes.isElemExisit(pickTra->id))
				{
					currentNewTrasNodes.insertElem(pickTra->id);
					pickTra->oldNodes = pickTra->nodes;
				}
				dijkstraMinConflicts(wavegroup.adjMatTraNums, pickTra, putback_delt, pickTra->nodes); // 将pickTra放下，获得完整解
				
				temp_add_delt = current_add_delt - pick_oldDelt - chain_delt + putback_delt;
				
				if (temp_add_delt < best_add_delt) {
					best_add_delt = temp_add_delt;
					//addTraNewTrasNodes = currentNewTrasNodes;
					/*addTraNewTrasNodes.delAllElem();
					for (auto i = 0; i < currentNewTrasNodes.Num; ++i) {
						addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
					}*/
					for (auto i = addTraNewTrasNodes.Num; i < currentNewTrasNodes.Num; ++i) {
						addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
					}
					for (auto i = 0; i < addTraNewTrasNodes.Num; ++i) {
						Traffic *t = &traffics[addTraNewTrasNodes.data[i]];
						t->best_changeNodes = t->nodes;
					}
					nobetterTimes = 0;
				}
				else if(temp_add_delt >= best_add_delt){
					if (cntCentProbChoice(4) && temp_add_delt == best_add_delt) {
						for (auto i = addTraNewTrasNodes.Num; i < currentNewTrasNodes.Num; ++i) {
								addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
							}
							for (auto i = 0; i < addTraNewTrasNodes.Num; ++i) {
								Traffic *t = &traffics[addTraNewTrasNodes.data[i]];
								t->best_changeNodes = t->nodes;
							}
					}
					nobetterTimes++;
				}
				len++;
				if (nobetterTimes >= 3) break;
			}

			int pick_newDelt;
			if(pickTra->nodes.size()==0)
				pickTra->nodes = pickTra->shorestNodes;
			wavegroup.addTra(pickTra); // 之后会进行recover
			wavegroup.deleteTraNotChangeGlobal(conflictTra);
			wavegroup.recoverFromOldNodes(traffics,currentNewTrasNodes, conflictTra);
			wavegroup.recordConflictTrasNotChangeGlobal(topo);//重新记录冲突
			
		}
		else {
			//addTraNewTrasNodes = currentNewTrasNodes;
			for (auto i = 0; i < currentNewTrasNodes.Num; ++i) {
				addTraNewTrasNodes.insertElem(currentNewTrasNodes.data[i]);
			}
			best_add_delt = 0;
		}
		return best_add_delt;
	}

	int RWAsolver::trydelTra_shaking(Traffic * delTra, TwoWayList & delBoxchangeTras)
	{
		//check_all(1);
		int del_delt, best_del_delt, current_del_delt;
		int orginal_w = delTra->wave;
		WaveGroup &wavegroup = waveGroups[orginal_w];
		del_delt = wavegroup.deleteTra(delTra, conflictTras);
		best_del_delt = current_del_delt = del_delt;
		delBoxchangeTras.delAllElem();
		delBoxchangeTras.insertElem(delTra->id); // 用于记录最优的业务改变情况
		currentNewTrasNodes.delAllElem();
		currentNewTrasNodes.insertElem(delTra->id);
		delTra->oldNodes = delTra->nodes;
		wavegroup.recordConflictTras(topo, conflictTras);
		if (wavegroup.conflictsOnAllLinks != 0) { 
			
			Traffic *pickTra = pickConflictTraffic(wavegroup);
			int pick_oldDelt = wavegroup.deleteTraNotChangeGlobal(pickTra); // 将pickTra的资源释放,得到部分解

			int len = 0;
			int chain_delt = 0; // 该ejectionChain的改进程度
			int nobetterTimes = 0;
			while (true) {
				int best_delt = 0, cnt = 1, randSelCnt = 1, randSel_delt = 0;
				Traffic *best_tra = NULL, *randSel_tra = NULL;
				List<ID> bestNodes, randSelNodes;
				List<ID> &groupConfTras = wavegroup.groupConflicTras.data;
				int groupConfNum, evaluateNum, groupTraNum;
				groupConfNum = wavegroup.groupConflicTras.Num;
				List <ID> evaluateIndexs;
				evaluateNum = min(3, groupConfNum);
				if (groupConfNum == 0) {
					break;
				}
				getKnumber(evaluateNum, groupConfNum, evaluateIndexs);
				for (auto i = 0; i < evaluateNum; ++i) {
					Traffic *tra;
					int index = evaluateIndexs[i];
					//int index = i; (groupConfNum == 0 && conflictsNum <= 3)?wavegroup.tras[index]:
					tra = &traffics[groupConfTras[index]];

					if (tra == pickTra) continue;
					int old_conflictNum = 0, new_conflictNum = 0;
					List<ID> reroutingNodes;
					int temp_delt = calculateIncomeForRerouting(wavegroup.adjMatTraNums, tra, reroutingNodes);
					if (temp_delt > best_delt) {// 说明部分解(pickTra未放下)
						best_delt = temp_delt;
						best_tra = tra;
						bestNodes = reroutingNodes;
						cnt = 1;
					}
					else if (temp_delt == best_delt) {
						cnt++;
						if (cntCentProbChoice(cnt)) {
							best_delt = temp_delt;
							best_tra = tra;
							bestNodes = reroutingNodes;
						}
					}
					if (cntCentProbChoice(randSelCnt)) {
						randSel_tra = tra;
						randSelNodes = reroutingNodes;
						randSel_delt = temp_delt;
					}
					randSelCnt++;
				}
				if (cntCentProbChoice(6)) { // 以较小的概率随机选择一个
					best_delt = randSel_delt;
					bestNodes = randSelNodes;
					best_tra = randSel_tra;
				}
				if (best_tra == NULL) {
					break;
				}
				if (!currentNewTrasNodes.isElemExisit(best_tra->id))
				{
					currentNewTrasNodes.insertElem(best_tra->id);
					best_tra->oldNodes = best_tra->nodes;
				}
				wavegroup.delTraNodes(best_tra);
				best_tra->nodes = bestNodes;
				wavegroup.addTraNodes(best_tra);
				wavegroup.recordConflictTrasNotChangeGlobal(topo);

				chain_delt += best_delt; // 改变best_tra得到的冲突数的减少量

										 // 判断是否能改进当前最优解
				int putback_delt = 0, temp_del_delt;
				if (!currentNewTrasNodes.isElemExisit(pickTra->id))
				{
					currentNewTrasNodes.insertElem(pickTra->id);
					pickTra->oldNodes = pickTra->nodes;
				}
				dijkstraMinConflicts(wavegroup.adjMatTraNums, pickTra, putback_delt, pickTra->nodes); // 将pickTra放下，获得完整解

				temp_del_delt = current_del_delt+pick_oldDelt+chain_delt - putback_delt;

				if (temp_del_delt > best_del_delt) {
					best_del_delt = temp_del_delt;
					for (auto i = delBoxchangeTras.Num; i < currentNewTrasNodes.Num; ++i) {
						delBoxchangeTras.insertElem(currentNewTrasNodes.data[i]);
					}
					for (auto i = 0; i < delBoxchangeTras.Num; ++i) {
						Traffic *t = &traffics[delBoxchangeTras.data[i]];
						t->best_changeNodes = t->nodes;
					}
					nobetterTimes = 0;
				}
				else if (temp_del_delt <= best_del_delt) {
					if (cntCentProbChoice(4) && temp_del_delt == best_del_delt) {
						for (auto i = delBoxchangeTras.Num; i < currentNewTrasNodes.Num; ++i) {
							delBoxchangeTras.insertElem(currentNewTrasNodes.data[i]);
						}
						for (auto i = 0; i < delBoxchangeTras.Num; ++i) {
							Traffic *t = &traffics[delBoxchangeTras.data[i]];
							t->best_changeNodes = t->nodes;
						}
					}
					nobetterTimes++;
				}
				len++;
				if (nobetterTimes >= 3) break;
			}

			if (pickTra->nodes.size() == 0)
				pickTra->nodes = pickTra->shorestNodes;
			wavegroup.addTra(pickTra); // 之后会进行recover
		}
		wavegroup.addTra(delTra);
		wavegroup.recoverFromOldNodes(traffics, currentNewTrasNodes);
		wavegroup.recordConflictTras(topo,conflictTras);//重新记录冲突
		//check_all(1);
		return best_del_delt;
	}
	int RWAsolver::delTraFromOriginalBox(Traffic *conflictTra, TwoWayList &delBoxchangeTras)
	{
		int del_delt;
		int orginal_w = conflictTra->wave;
		del_delt = waveGroups[orginal_w].deleteTra(conflictTra, conflictTras);
		delBoxchangeTras.delAllElem();
		delBoxchangeTras.insertElem(conflictTra->id);
		conflictTra->oldNodes = conflictTra->nodes;
		waveGroups[orginal_w].recordConflictTras(topo, conflictTras); //统计冲突业务
		if (waveGroups[orginal_w].conflictsOnAllLinks - del_delt == 0) { return del_delt; }

		int noBetterTimes = 0;
		while (true) {
			ReroutingMove rrMove;
			findBestRerouting(waveGroups[orginal_w], waveGroups[orginal_w].groupConflicTras, rrMove);
			if (rrMove.tra == NULL) { break; }
			if (!delBoxchangeTras.isElemExisit(rrMove.tra->id)) {
				delBoxchangeTras.insertElem(rrMove.tra->id);
				rrMove.tra->oldNodes = rrMove.tra->nodes;
			}
			waveGroups[orginal_w].delTraNodes(rrMove.tra);
			rrMove.tra->best_changeNodes = rrMove.tra->nodes = rrMove.new_nodes;
			waveGroups[orginal_w].addTraNodes(rrMove.tra);

			//addChangedTrasNodes(delBoxNewTrasNodes, rrMove.tra);
			waveGroups[orginal_w].recordConflictTras(topo, conflictTras);
			del_delt += rrMove.delt;
			if (rrMove.delt == 0) { noBetterTimes++; }
			else if (rrMove.delt > 0) {
				noBetterTimes = 0;
			}
			if (noBetterTimes == 3) {
				break;
			}
		}
		return del_delt;
	}
	
	Traffic * RWAsolver::pickConflictTraffic(WaveGroup & wavegroup, Traffic * addedTra)
	{
		if (cntCentProbChoice(4)) {
			return pickRandomConflictTraffic(wavegroup, addedTra);
		}
		else {
			return pickMostConflictTraffic(wavegroup, addedTra);
		}
	}

	Traffic * RWAsolver::pickMostConflictTraffic(WaveGroup & wavegroup, Traffic * addedTra)
	{
		int maxConflict = 0;
		Traffic *pickTra=NULL;
		List<ID> &groupConfTras = wavegroup.groupConflicTras.data;
		int groupConfNum = wavegroup.groupConflicTras.Num;
		for (auto i = 0; i < groupConfNum; ++i) {
			Traffic *tra = &traffics[groupConfTras[i]];
			if (tra != addedTra) {
				int conflictNum = 0;
				for (auto j = 0; j < tra->nodes.size() - 1; ++j) {
					ID node1 = tra->nodes[j], node2 = tra->nodes[j + 1];
					if (wavegroup.adjMatTraNums[node1][node2] > 1) {
						conflictNum++;
					}
				}
				if (conflictNum > maxConflict) {
					maxConflict = conflictNum;
					pickTra = tra;
				}
			}
		}
		return pickTra;
	}

	Traffic * RWAsolver::pickRandomConflictTraffic(WaveGroup & wavegroup, Traffic * addedTra)
	{
		if (wavegroup.groupConflicTras.Num == 0)return NULL;
		while (true)
		{
			int index = rand() % wavegroup.groupConflicTras.Num;
			Traffic *pickTra = &traffics[wavegroup.groupConflicTras.data[index]];
			if (pickTra != addedTra) {
				return pickTra;
			}
		}
	}
	bool pair_compare(const pair<ID, int> &p1, const pair<ID, int> &p2) {
		return p1.second < p2.second;
	}
	bool pair_compare_increase(const pair<ID, pair<int,int>> &p1, const pair<ID, pair<int,int>> &p2) {
		int t1 = p1.second.first, t2 = p2.second.first;
		if (t1< t2 ) return true;
		else if (t1 == t2) return p1.second.second < p2.second.second;
		return false;
	}
	bool simple_compare_increase(const pair<ID, pair<int, int>> &p1, const pair<ID, pair<int, int>> &p2) {
		int t1 = p1.second.first, t2 = p2.second.first;
		if (t1 < t2) return true;
		else if (t1 == t2 && cntCentProbChoice(2)) return true;
		return false;
	}
	bool simple_traffic_compare_increase(const pair<Traffic*, pair<int, int>> &p1, const pair<Traffic *, pair<int, int>> &p2) {
		int t1 = p1.second.first, t2 = p2.second.first;
		if (t1< t2) return true;
		else if (t1 == t2 && cntCentProbChoice(2)) return true;
		return false;
	}
	bool binValueCompare_increase_leaveTime(const pair<ID, pair<int, int>> &p1, const pair<ID, pair<int, int>> &p2) {
		int t1 = p1.second.first, t2 = p2.second.first;
		if (t1< t2) return true;
		else if (t1 == t2) {
			if(p1.second.second < p2.second.second 
				||( p1.second.second == p2.second.second && cntCentProbChoice(2))) return true;
		}
		return false;
	}

	bool traffic_pair_compare_increase_leaveTime(const pair<Traffic*, pair<int, int>> &p1, const pair<Traffic *, pair<int, int>> &p2) {
		int t1 = p1.second.first, t2 = p2.second.first;
		if (t1< t2) return true;
		else if (t1 == t2) return p1.second.second < p2.second.second;
		return false;
	}
	void RWAsolver::layeredFind_move()
	{
		int non_tabu_cnt = 1;
		Traffic *conflictTra = getOneConflictTra();
		int original_w = conflictTra->wave;
		int confTraId = conflictTra->id;
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		int best_w = 0;

		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		int del_delt = delTraFromOriginalBox(conflictTra, delBoxchangeTras);
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		// del_delt为业务从原颜色盒子移除减少的冲突数
		int cnt = 1;
		int add_delt;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<ID> valuedWave;
		valuedWave.reserve(WaveNum);
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {

				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
					delta[confTraId][new_w] = add_delt;
				}
				else {
					add_delt = delta[confTraId][new_w];
				}
				int temp_delt = del_delt - add_delt;
				if (temp_delt >= 0 && conflictsNum == 1) {
					valuedWave.push_back(new_w);
				}
				if (temp_delt > best_delt) {
					non_tabu_cnt = 1;
					best_delt = temp_delt;
					best_w = new_w;
				}
				else if (temp_delt > sBest_delt) {
					nonTabu_sBest_cnt = 1;
					sBest_delt = temp_delt;
					sBest_w = new_w;
				}
				else if (temp_delt == best_delt) {
					non_tabu_cnt++;
					if (cntCentProbChoice(non_tabu_cnt)) {
						best_w = new_w;
					}
				}
				else if (temp_delt == sBest_delt) {
					nonTabu_sBest_cnt++;
					if (cntCentProbChoice(nonTabu_sBest_cnt)) {
						sBest_w = new_w;
					}
				}
			}

		}
		if (best_delt > -2) {// 尝试用ejection chain使得冲突减少量增大
			int old_best = best_delt;
			best_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[best_w], best_newTrasNodes);

			if (sBest_delt > -2 && sBest_w != best_w) {
				List<ID> oldBest_changeNodes = conflictTra->best_changeNodes;
				int temp_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[sBest_w], tempNewNodes);
				if ((temp_delt > best_delt&& temp_delt >= 0) || (temp_delt == best_delt && temp_delt >= 0 && cntCentProbChoice(3))) {

					best_w = sBest_w;
					best_delt = temp_delt;
					best_newTrasNodes.delAllElem();
					for (auto i = 0; i < tempNewNodes.Num; ++i) {
						best_newTrasNodes.insertElem(tempNewNodes.data[i]);
					}
				}
				else {
					conflictTra->best_changeNodes = oldBest_changeNodes;
				}
			}

		}

		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb))
			) {
			if (best_newTrasNodes.Num == 0) {
				dijkstraMinConflicts(waveGroups[best_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
				best_delt = del_delt - add_delt;
				conflictTra->best_changeNodes = conflictTra->nodes = temp_nodes;
				best_newTrasNodes.insertElem(conflictTra->id);
			}
			make_layerMove(original_w, best_w, best_newTrasNodes, conflictTra);
			effectIterNum++;
		}
		else {
			bool change = false;
			if (!change) {
				waveGroups[conflictTra->wave].addTra(conflictTra);
				waveGroups[conflictTra->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
				waveGroups[conflictTra->wave].recordConflictTras(topo, conflictTras);
				check_all(1);

			}
		}
	}
	void RWAsolver::exchangeTraffic_shaking()
	{
		//check_all(1);
		Traffic *traA = getOneConflictTra(); // 获取一条业务
		int waveA = traA->wave;
		List<ID> oldNodesA = traA->nodes;
		ID confTraId = traA->id;
		struct exchangeMove {
			Traffic *traB;
			List<ID> pathB;
		}best_move;
		if (iteration == 1 &&WaveNum == 69) {
			cout << "test\n";
			check_all(1);
		}
		int del_deltA = waveGroups[waveA].deleteTra(traA, conflictTras); // 将业务A从原来的盒子中删除
		best_delt =MyIntMin;
		int best_add_deltA = INT_MAX;
		int add_deltA;
		
		List<pair<ID, pair<int,int>>> bin_value;
		//List<pair<ID, int>> bin_value;
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != waveA) {
				List<ID> temp_nodes;
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, traA, add_deltA, temp_nodes);
					delta[confTraId][new_w] = add_deltA;
				}
				else {
					add_deltA = delta[confTraId][new_w];
				}
				bin_value.push_back(make_pair(new_w, make_pair(add_deltA,leaveTime[traA->id][new_w])));
				//bin_value.push_back(make_pair(new_w, add_deltA));
			}
		}
		int m = env.mBest;
		//get_mBest_bin(bin_value, m);
		partial_sort(bin_value.begin(), bin_value.begin() + m, bin_value.end(), pair_compare_increase);

		int cnt = 1;
		List<List<ID>> bestGroupANodes;
		// 从m个盒子中挑选出业务进行交换
		Traffic *bestTraB = NULL;
		bestTraB = bestExchangeMoveInMbins(traA, del_deltA, bestGroupANodes, bin_value);
		// try make_move
		make_exchangeMove(traA,oldNodesA,bestTraB, bestGroupANodes);
	}

	void RWAsolver::selectKtraffics(List<Traffic *> &ptraffics, int k, List<Traffic *> &kTraffics,ID waveA)
	{
		int curIplus1 = 0;
		for (auto i = 0; i < ptraffics.size(); i++) { // 随机选取k条业务进行交换动作的尝试
			Traffic *tra = ptraffics[i];
			if (tra->wave == waveA) continue;
			curIplus1++;
			if (kTraffics.size() < k) {
				kTraffics.push_back(tra);
				continue;
			}
			int index = rand() % curIplus1;
			if (index < k) { // 使得每一个被选中的概率为 k/curIplus1，最终为k/sum;
				kTraffics[index] = tra;
			}
		}
	}

	void RWAsolver::obtainMbins(Traffic *traA, int m, List<std::pair<ID, std::pair<int, int>>>& bin_value)
	{
		int add_deltA;
		ID waveA = traA->wave;
		int confTraId = traA->id;
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != waveA) {
				List<ID> temp_nodes;
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, traA, add_deltA, temp_nodes);
					delta[confTraId][new_w] = add_deltA;
				}
				else {
					add_deltA = delta[confTraId][new_w];
				}
				bin_value.push_back(make_pair(new_w, make_pair(add_deltA, leaveTime[traA->id][new_w])));
			}
		}
		partial_sort(bin_value.begin(), bin_value.begin() + m, bin_value.end(), simple_compare_increase);
	}

	void RWAsolver::exchangeTraffic_all()
	{
		Traffic *traA = getOneConflictTra(); // 获取一条业务
		if (traA == NULL) return;
		int waveA = traA->wave;
		List<ID> oldNodesA = traA->nodes;
		ID confTraId = traA->id;
		Traffic *bestTraB = NULL;
		WaveGroup &wavegroupA = waveGroups[waveA];
		int del_deltA = waveGroups[waveA].deleteTra(traA,conflictTras); // 将业务A从原来的盒子中删除
		int m = env.mBest;
		int best_delt = MyIntMin;
		List<pair<ID, pair<int, int>>> bin_value;
		obtainMbins(traA, m, bin_value);
		List<Traffic *> kTraffics;
		for (int i = 0; i < m; i++) {
			int wave = bin_value[i].first;
			WaveGroup &wavegroupB = waveGroups[wave];
			Set<ID> groupBconfTras;
			for (int j = 0; j < wavegroupB.groupConflicTras.Num; j++) {
				groupBconfTras.insert(wavegroupB.groupConflicTras[j]);
			}
			wavegroupB.addTra(traA); // 将业务A插入到盒子B 
			wavegroupB.recordConflictTrasNotChangeGlobal(topo); // 更新冲突业务
			for (int j = 0; j < wavegroupB.groupConflicTras.Num; j++) {
				Traffic *temp = ptraffics[wavegroupB.groupConflicTras[j]];
				if (temp != traA //&& groupBconfTras.find(temp->id) == groupBconfTras.end()
					)
					kTraffics.push_back(temp);
			}
			wavegroupB.deleteTra(traA, conflictTras);
			wavegroupB.groupConflicTras.delAllElem();
			for (auto iter = groupBconfTras.begin(); iter != groupBconfTras.end(); iter++){
				wavegroupB.groupConflicTras.insertElem(*iter);
			}
			//wavegroupB.recordConflictTrasNotChangeGlobal(topo);
		}
		List<pair<Traffic *, pair<int, int>>> traffic_value;

		for (auto i = 0; i < kTraffics.size(); i++) {
			Traffic *traB = kTraffics[i];
			if (waveA == traB->wave || (traA->src == traB->src && traA->dst == traB->dst)) continue;
			WaveGroup &wavegroupB = waveGroups[traB->wave];
			int del_deltB = wavegroupB.deleteTraWithoutChangeConflict(traB);
			int add_deltA, add_deltB;
			List<ID> nodesA, nodesB;
			dijkstraMinConflicts(wavegroupB.adjMatTraNums, traA, add_deltA, nodesA);
			dijkstraMinConflicts(wavegroupA.adjMatTraNums, traB, add_deltB, nodesB);
			wavegroupB.addTra(traB);
			int temp_delt = add_deltA + add_deltB - (del_deltA + del_deltB);
			traffic_value.push_back(make_pair(traB, make_pair(temp_delt, leaveTime[traB->id][waveA])));
		}
		//m = 1;
		m = min((int)traffic_value.size(), m);
		partial_sort(traffic_value.begin(), traffic_value.begin() + m, traffic_value.end(), simple_traffic_compare_increase);
		/*for (size_t i = 0; i < m; i++){
			cout << traffic_value[i].second.first <<endl;
		}*/
		if (traffic_value.size() == 0) {
			traA->nodes = oldNodesA;
			waveGroups[waveA].addTra(traA);
			waveGroups[waveA].recordConflictTras(topo, conflictTras);
			switchNeighType(neighType);
			return;
		}
		int cnt = 1;
		List<ID> conflitTra_bestNodes;
		List<List<ID>> bestGroupANodes;
		List<List<ID>> bestGroupBNodes;
		if (traffic_value[0].second.first <=2) {
			for (int i = 0; i < m; i++) {
				Traffic *traB = traffic_value[i].first;
				if (traffic_value[i].second.first >= 5) continue;
				if (iteration == 137) {
					cout << traB->id << " " << traB->wave << endl;
				}
				int waveB = traB->wave;
				WaveGroup &wavegroupB = waveGroups[waveB];
				int del_deltB = wavegroupB.deleteTra(traB, conflictTras);
				int add_deltA = tryAddTraToNewBoxByEjectionChain_Grasp(traA, wavegroupB, tempNewNodes);
				List<ID> oldNodesB = traB->nodes;
				int add_deltB = tryAddTraToNewBoxByEjectionChain_Grasp(traB, wavegroupA, tempNewNode2);
				traB->nodes = oldNodesB;
				wavegroupB.addTra(traB);
				wavegroupB.recordConflictTras(topo, conflictTras);
				int temp_delt = del_deltA + del_deltB - add_deltA - add_deltB;
				if (temp_delt > best_delt) {
					best_delt = temp_delt;
					bestTraB = traB;
					best_delBoxchangeTras = tempNewNodes;// 盒子B
					//tempNewNodes.delElem(traA->id),tempNewNodes.insertElem(traB->id);
					best_newTrasNodes = tempNewNode2; // 盒子A
					//tempNewNode2.delElem(traB->id), tempNewNode2.insertElem(traA->id);
					bestGroupANodes.clear(), bestGroupBNodes.clear();
					for (auto i = 0; i < best_newTrasNodes.Num; i++) {
						Traffic *tempTra = &traffics[best_newTrasNodes.data[i]];
						bestGroupANodes.push_back(tempTra->best_changeNodes);
					}
					for (auto i = 0; i < best_delBoxchangeTras.Num; i++) {
						Traffic *tempTra = &traffics[best_delBoxchangeTras.data[i]];
						bestGroupBNodes.push_back(tempTra->best_changeNodes);
					}
					cnt = 1;
				}
				else if (temp_delt == best_delt) {
					cnt++;
					if (cntCentProbChoice(cnt)) {
						bestTraB = traB;
						best_delBoxchangeTras = tempNewNodes;// 盒子B
						//tempNewNodes.delElem(traA->id), tempNewNodes.insertElem(traB->id);
						best_newTrasNodes = tempNewNode2; // 盒子A
						//tempNewNode2.delElem(traB->id), tempNewNode2.insertElem(traA->id);
						bestGroupANodes.clear(), bestGroupBNodes.clear();
						for (auto i = 0; i < best_newTrasNodes.Num; i++) {
							Traffic *tempTra = &traffics[best_newTrasNodes.data[i]];
							bestGroupANodes.push_back(tempTra->best_changeNodes);
						}
						for (auto i = 0; i < best_delBoxchangeTras.Num; i++) {
							Traffic *tempTra = &traffics[best_delBoxchangeTras.data[i]];
							bestGroupBNodes.push_back(tempTra->best_changeNodes);
						}
					}
				}
				if (iteration == 140) {
					check_all(1);
				}
			}
		}
		// try make_move

		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb))) {
			Traffic *traB = bestTraB;
			ID waveB = traB->wave;
			//cout << waveB<<" "<<waveA << endl;
			WaveGroup &wavegroupB = waveGroups[waveB];
			WaveGroup &wavegroupA = waveGroups[waveA];
			wavegroupB.deleteTra(traB, conflictTras);
			wavegroupB.addTra(traA);
			wavegroupA.addTra(traB);

			for (auto i = 0; i < best_newTrasNodes.Num; i++) { // 在盒子A中的业务
				Traffic *tempTra = &traffics[best_newTrasNodes.data[i]];
				tempTra->nodes = bestGroupANodes[i];
			}
			for (auto i = 0; i < best_delBoxchangeTras.Num; i++) {
				Traffic *tempTra = &traffics[best_delBoxchangeTras.data[i]];
				//cout << tempTra->id << endl;
				tempTra->nodes = bestGroupBNodes[i];
			}
			groupChangedIter[waveA] = iteration;
			groupChangedIter[waveB] = iteration;
			leaveTime[traA->id][waveA] = iteration;
			leaveTime[traB->wave][waveB] = iteration;
			traA->wave = waveB;
			traB->wave = waveA;
			conflictsNum -= best_delt;
			waveGroups[waveA].afreshAdjMat(topo.adjMat);
			waveGroups[waveA].recordConflictTras(topo, conflictTras);
			waveGroups[waveB].afreshAdjMat(topo.adjMat);
			waveGroups[waveB].recordConflictTras(topo, conflictTras);
			check_all(1);
		}
		else {
			traA->nodes = oldNodesA;
			waveGroups[waveA].addTra(traA);
			waveGroups[waveA].recordConflictTras(topo, conflictTras);
			check_all(1);

		}


	}

	
	void RWAsolver::bestShiftMoveInMbins(Traffic *traA,int &best_w,int del_deltA,List<pair<ID, pair<int, int>>> &bin_value)
	{
		int temp_best_delt = MyIntMin;
		int cnt = 1;
		List<ID> conflitTra_bestNodes;
		for (int i = 0; i < env.mBest; i++) {
			int cur_w = bin_value[i].first;
			//if (bin_value[i].second.first < -3) continue;
			tempNewNodes.delAllElem();
			mCnt = i;
			int cur_delt = del_deltA - tryAddTraToNewBoxByEjectionChain_Grasp(traA, waveGroups[cur_w], tempNewNodes,true);
			if (cur_delt > temp_best_delt) {
				temp_best_delt = cur_delt;
				best_w = cur_w;
				cnt = 1;
				conflitTra_bestNodes = traA->best_changeNodes;
				best_newTrasNodes = tempNewNodes;
			}
			else if (cur_delt == temp_best_delt) {
				cnt++;
				if (cntCentProbChoice(cnt)) {
					best_w = cur_w;
					conflitTra_bestNodes = traA->best_changeNodes;
					best_newTrasNodes = tempNewNodes;
				}
			}
		}
		traA->best_changeNodes = conflitTra_bestNodes;
		best_delt = temp_best_delt;
	}

	Traffic* RWAsolver::bestExchangeMoveInMbins(Traffic *traA, int del_deltA,List<List<ID>> &bestGroupANodes, List<pair<ID, pair<int, int>>> &bin_value)
	{
		int add_deltA;
		int cnt = 1;
		best_delt = MyIntMin;
		Traffic *bestTraB = NULL;
		for (int i = 0; i < env.mBest; i++) {
			int wave = bin_value[i].first;
			//if (bin_value[i].second.first < -3) continue;

			WaveGroup &wavegroupB = waveGroups[wave];
			if (greedy_deltNodes.size() != 0) {
				add_deltA = greedy_deltNodes[i].first;
				traA->nodes = greedy_deltNodes[i].second;
			}else
				dijkstraMinConflicts(wavegroupB.adjMatTraNums, traA, add_deltA, traA->nodes); // 计算将业务A添加到盒子B的冲突的增量
			if (add_deltA == 0) evaluate_cnt++;
			wavegroupB.addTra(traA); // 将业务A插入到盒子B 
			traA->best_changeNodes = traA->nodes;
			wavegroupB.recordConflictTras(topo, conflictTras); // 更新冲突业务
			if (wavegroupB.conflictsOnAllLinks != 0) {

				Traffic *traB = pickConflictTraffic(wavegroupB, traA); // 从冲突业务中选择一条与业务A不相同的业务
				//int del_deltB = trydelTra_shaking(traB, delBoxchangeTras); // 尝试将业务B从其盒子中删除,同时摇一摇
				if ( !(traB->src == traA->src && traB->dst == traA->dst)) {
					int del_deltB = delTraFromOriginalBox(traB, delBoxchangeTras);
					wavegroupB.addTra(traB);
					wavegroupB.recoverFromOldNodes(traffics, delBoxchangeTras);
					wavegroupB.recordConflictTras(topo, conflictTras);
					List<ID> oldNodeB = traB->nodes;
					int add_deltB = tryAddTraToNewBoxByEjectionChain_Grasp(traB, waveGroups[traA->wave], tempNewNodes); // 尝试将业务B添加入业务A所在的盒子，同时摇一摇
					traB->nodes = oldNodeB;
					int temp_delt = del_deltA + del_deltB - add_deltA - add_deltB;
					if (temp_delt > best_delt) {
						best_delt = temp_delt;
						bestTraB = traB;
						best_delBoxchangeTras = delBoxchangeTras;
						best_newTrasNodes = tempNewNodes;
						bestGroupANodes.clear();
						bestGroupANodes.push_back(traA->best_changeNodes);
						for (auto i = 0; i < best_newTrasNodes.Num; i++) {
							Traffic *tempTra = &traffics[best_newTrasNodes.data[i]];
							bestGroupANodes.push_back(tempTra->best_changeNodes);
						}
						cnt = 1;
					}
					else if (temp_delt == best_delt) {
						cnt++;
						if (cntCentProbChoice(cnt)) {
							bestTraB = traB;
							best_delBoxchangeTras = delBoxchangeTras;
							best_newTrasNodes = tempNewNodes;
							bestGroupANodes.clear();
							bestGroupANodes.push_back(traA->best_changeNodes);
							for (auto i = 0; i < best_newTrasNodes.Num; i++) {
								Traffic *tempTra = &traffics[best_newTrasNodes.data[i]];
								bestGroupANodes.push_back(tempTra->best_changeNodes);
							}
						}
					}
				}
			}

			// 恢复现场 
			wavegroupB.deleteTra(traA, conflictTras);
			//wavegroupB.recoverFromOldNodes(traffics, currentNewTrasNodes, traA);
			wavegroupB.recordConflictTras(topo, conflictTras);//重新记录冲突

		}
		return bestTraB;
	}

	void RWAsolver::merged_shift_exchange(){
		Traffic *traA = getOneConflictTra(); 
		if (traA == NULL) return;
		int waveA = traA->wave;
		int confTraId = traA->id;
		int best_w = 0;
		if (iteration == 29) {
			cout << "test" << endl;
		}
		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		List<ID> oldNodesA = traA->nodes;
		int del_deltA = delTraFromOriginalBox(traA, delBoxchangeTras);
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		int cnt = 1;
		int add_deltA;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<pair<ID, pair<int, int>>> bin_value;
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != waveA) {
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, traA, add_deltA, temp_nodes);
					delta[confTraId][new_w] = add_deltA;
				}
				else {
					add_deltA = delta[confTraId][new_w];
				}
				bin_value.push_back(make_pair(new_w, make_pair(add_deltA, leaveTime[traA->id][new_w])));
			}
		}
		int m = env.mBest;
		//get_mBest_bin(bin_value, m);
		partial_sort(bin_value.begin(), bin_value.begin() + m, bin_value.end(), pair_compare_increase);
		best_delt = del_deltA - bin_value[0].second.first;
		greedy_deltNodes.clear();
		if (best_delt > -2) {
			bestShiftMoveInMbins(traA, best_w, del_deltA, bin_value);
		}
		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb) //&& noBeterTimes >= 300
			)) {
			make_layerMove(waveA, best_w, best_newTrasNodes, traA);
			effectIterNum++;
		}
		else {
			
			waveGroups[traA->wave].addTra(traA);
			waveGroups[traA->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
			waveGroups[traA->wave].recordConflictTras(topo, conflictTras);
			if (best_delt > -2) {
				Traffic *bestTraB;
				List<List<ID>> bestGroupANodes;
				del_deltA = waveGroups[traA->wave].deleteTra(traA, conflictTras);
				bestTraB = bestExchangeMoveInMbins(traA, del_deltA, bestGroupANodes, bin_value);
				make_exchangeMove(traA, oldNodesA, bestTraB, bestGroupANodes);
			}
			//if (best_delt >= -2) {
			check_all(1);
			//}
		}
	}
	void RWAsolver::make_exchangeMove(Traffic *traA,List<ID> oldNodesA, Traffic *bestTraB,List<List<ID>> &bestGroupANodes) {
		ID waveA = traA->wave;
		if (bestTraB &&(best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb)))) {
			Traffic *traB = bestTraB;
			ID waveB = traB->wave;
			//cout << waveB<<" "<<waveA << endl;
			WaveGroup &wavegroupB = waveGroups[waveB];
			WaveGroup &wavegroupA = waveGroups[waveA];
			wavegroupB.deleteTra(traB, conflictTras);
			wavegroupB.addTra(traA);
			wavegroupA.addTra(traB);

			for (auto i = 0; i < best_newTrasNodes.Num; i++) {
				Traffic *tempTra = &traffics[best_newTrasNodes.data[i]];
				tempTra->nodes = bestGroupANodes[i + 1];
			}
			traA->nodes = bestGroupANodes[0];
			for (auto i = 0; i < best_delBoxchangeTras.Num; i++) {
				Traffic *tempTra = &traffics[best_delBoxchangeTras.data[i]];
				if (tempTra != traA) {
					tempTra->nodes = tempTra->best_changeNodes;
				}
			}
			groupChangedIter[waveA] = iteration;
			groupChangedIter[waveB] = iteration;
			leaveTime[traA->id][waveA] = iteration;
			leaveTime[traB->wave][waveB] = iteration;
			traA->wave = waveB;
			traB->wave = waveA;
			conflictsNum -= best_delt;
			waveGroups[waveA].afreshAdjMat(topo.adjMat);
			waveGroups[waveA].recordConflictTras(topo, conflictTras);
			waveGroups[waveB].afreshAdjMat(topo.adjMat);
			waveGroups[waveB].recordConflictTras(topo, conflictTras);
			check_all(1);
		}
		else {
			traA->nodes = oldNodesA;
			waveGroups[waveA].addTra(traA);
			waveGroups[waveA].recordConflictTras(topo, conflictTras);
			check_all(1);

		}
	}
	void RWAsolver::exchangeTraffic()
	{
		Traffic *traA = getOneConflictTra(); // 获取一条业务
		int waveA = traA->wave;
		List<ID> oldNodesA = traA->nodes;
		ID confTraId = traA->id;
		struct exchangeMove {
			Traffic *traB;
			List<ID> pathB;
		}best_move;
		int del_deltA = waveGroups[waveA].deleteTra(traA, conflictTras); // 将业务A从原来的盒子中删除
		int best_delt = MyIntMin;
		int best_add_deltA = INT_MAX;
		int add_deltA;

		List<pair<ID, pair<int, int>>> bin_value;
		//List<pair<ID, int>> bin_value;
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != waveA) {
				List<ID> temp_nodes;
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, traA, add_deltA, temp_nodes);
					delta[confTraId][new_w] = add_deltA;
				}
				else {
					add_deltA = delta[confTraId][new_w];
				}
				bin_value.push_back(make_pair(new_w, make_pair(add_deltA, leaveTime[traA->id][new_w])));
				//bin_value.push_back(make_pair(new_w, add_deltA));
			}
		}
		int m = env.mBest;
		//get_mBest_bin(bin_value, m);
		partial_sort(bin_value.begin(), bin_value.begin() + m, bin_value.end(), simple_compare_increase);

		int cnt = 1;
		// 先选出m个盒子
		for (int i = 0; i < m; i++) {
			int wave = bin_value[i].first;
			WaveGroup &wavegroupB = waveGroups[wave];
			dijkstraMinConflicts(wavegroupB.adjMatTraNums, traA, add_deltA, traA->nodes); // 计算将业务A添加到盒子B的冲突的增量
			wavegroupB.addTra(traA); // 将业务A插入到盒子B 
			wavegroupB.recordConflictTrasNotChangeGlobal(topo); // 更新冲突业务
			bool flag = false;
			//if (wavegroupB.groupConflicTras.Num != 0) flag = true;
			for (int j = 0; j < wavegroupB.groupConflicTras.Num; j++) {
				//if (flag && cntCentProbChoice(2)) continue;
				Traffic *traB = &traffics[wavegroupB.groupConflicTras.data[j]];
				if (traA == traB) continue;
				List<ID> nodesB;
				int del_deltB = wavegroupB.tryDelTra(traB), add_deltB;
				dijkstraMinConflicts(waveGroups[waveA].adjMatTraNums, traB, add_deltB, nodesB);
				int temp_delt = del_deltA + del_deltB - add_deltA - add_deltB;
				if (temp_delt>best_delt) {
					best_delt = temp_delt;
					best_move.pathB = nodesB;
					best_move.traB = traB;
					cnt = 1;
				}
				else if (temp_delt == best_delt) {
					cnt++;
					if (cntCentProbChoice(cnt)) {
						best_move.pathB = nodesB;
						best_move.traB = traB;
					}
				}

			}
			// 恢复现场 
			wavegroupB.deleteTra(traA, conflictTras);
			//wavegroupB.recoverFromOldNodes(traffics, currentNewTrasNodes, traA);
			wavegroupB.recordConflictTrasNotChangeGlobal(topo);//重新记录冲突
															   //check_all(1);
		}

		// try make_move
		if (best_delt >= -2) {

			tempNewNodes.delAllElem();
			ID waveB = best_move.traB->wave;
			WaveGroup &wavegroupB = waveGroups[waveB];
			WaveGroup &wavegroupA = waveGroups[waveA];
			int del_deltB = wavegroupB.deleteTra(best_move.traB, conflictTras);
			List<ID> oldNodesB = best_move.traB->nodes;
			int add_deltA = tryAddTraToNewBoxByEjectionChain_Grasp(traA, wavegroupB, tempNewNodes);
			tempNewNode2.delAllElem();
			int add_deltB = tryAddTraToNewBoxByEjectionChain_Grasp(best_move.traB, wavegroupA, tempNewNode2);
			int delt = del_deltA + del_deltB - add_deltA - add_deltB;
			if (delt >= 0 || (delt == -1 && cntCentProbChoice(env.worseSolProb))) {
				traA->wave = waveB;
				leaveTime[traA->id][waveA] = iteration;
				best_move.traB->wave = waveA;
				leaveTime[best_move.traB->wave][waveB] = iteration;
				wavegroupA.addTra(best_move.traB);
				wavegroupB.addTra(traA);
				make_exchangeMove(waveA, waveB, tempNewNodes, tempNewNode2);
				conflictsNum -= delt;
			}
			else {
				traA->nodes = oldNodesA;
				best_move.traB->nodes = oldNodesB;
				wavegroupA.addTra(traA);
				wavegroupB.addTra(best_move.traB);
				wavegroupA.recordConflictTras(topo, conflictTras);
				wavegroupB.recordConflictTras(topo, conflictTras);
			}
			check_all(1);
		}
		else {
			traA->nodes = oldNodesA;
			waveGroups[waveA].addTra(traA);
			waveGroups[waveA].recordConflictTras(topo, conflictTras);
		}


	}
	void RWAsolver::get_mBest_bin(List<pair<ID, int>>& binValue,int m){
		binValueCnt.resize(20);
		List<pair<int, int>> index(20);
		partial_sort(binValue.begin(),binValue.begin()+m, binValue.end(), pair_compare);
		bool start = true;
		for (int i = 0; i < m; i++) {
			int value = binValue[i].second;
			binValueCnt[value] ++;
			if (start) {
				index[value].first = i;
				start = false;
			}
			if (i == m - 1 || value != binValue[i + 1].second) {
				index[value].second = i;
				start = true;
			}
		}
		for (int i = m; i < binValue.size(); i++) {
			int value = binValue[i].second;
			if (binValueCnt[value] != 0) {
				binValueCnt[value] ++;
				if (cntCentProbChoice(binValueCnt[value])) {
					int tempIndex = index[value].first + (rand() % (index[value].second - index[value].first + 1));
					//pair<ID, int> temp = binValue[tempIndex];
					binValue[tempIndex] = binValue[i];
				}
			}
		}
	}
	void RWAsolver::layeredFind_move_m_best()
	{
		if (iteration == 36 && WaveNum == 68) {
			cout << "test\n";
			cout << conflictTras.Num << endl;
		}
		int non_tabu_cnt = 1;
		Traffic *conflictTra = getOneConflictTra();
		if (conflictTra == NULL) return;
		int original_w = conflictTra->wave;
		int confTraId = conflictTra->id;
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		int best_w = 0;
		
		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		List<ID> conflictTraOldNodes = conflictTra->nodes;
		int del_delt = trydelTra_shaking(conflictTra, delBoxchangeTras);
			//delTraFromOriginalBox(conflictTra, delBoxchangeTras);
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		// del_delt为业务从原颜色盒子移除减少的冲突数
		int cnt = 1;
		int add_delt;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<ID> valuedWave;
		List<pair<ID, pair<int, int>>> bin_value;

		//List<pair<ID, int>> bin_value;
		valuedWave.reserve(WaveNum);
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
					delta[confTraId][new_w] = add_delt;
				}
				else {
					add_delt = delta[confTraId][new_w];
				}
				//int temp_delt = del_delt - add_delt;
				bin_value.push_back(make_pair(new_w, make_pair(add_delt, leaveTime[conflictTra->id][new_w])));

				//bin_value.push_back(make_pair(new_w, add_delt));
			}
		}
		int m = env.mBest;
		//get_mBest_bin(bin_value, m);
		partial_sort(bin_value.begin(), bin_value.begin() + m, bin_value.end(), pair_compare_increase);
		best_delt = del_delt - bin_value[0].second.first;
		if (best_delt >= -2) {
			int temp_best_delt = MyIntMin;
			int cnt = 1;
			List<ID> conflitTra_bestNodes;
			for (int i = 0; i < m; i++) {
				int cur_w = bin_value[i].first;
				tempNewNodes.delAllElem();
				int cur_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[cur_w], tempNewNodes);
				if (cur_delt > temp_best_delt) {
					temp_best_delt = cur_delt;
					best_w = cur_w;
					cnt = 1;

					conflitTra_bestNodes = conflictTra->best_changeNodes;
					best_newTrasNodes.delAllElem();
					for (auto i = 0; i < tempNewNodes.Num; ++i) {
						best_newTrasNodes.insertElem(tempNewNodes.data[i]);
					}
				}
				else if (cur_delt == temp_best_delt) {
					cnt++;
					if (cntCentProbChoice(cnt)) {
						best_w = cur_w;
						conflitTra_bestNodes = conflictTra->best_changeNodes;

						best_newTrasNodes.delAllElem();
						for (auto i = 0; i < tempNewNodes.Num; ++i) {
							best_newTrasNodes.insertElem(tempNewNodes.data[i]);
						}
					}
				}
			}
			conflictTra->best_changeNodes = conflitTra_bestNodes;
			best_delt = temp_best_delt;
		}

		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb) //&& noBeterTimes >= 300
			)) {
			if (best_newTrasNodes.Num == 0) {
				dijkstraMinConflicts(waveGroups[best_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
				best_delt = del_delt - add_delt;
				conflictTra->best_changeNodes = conflictTra->nodes = temp_nodes;
				best_newTrasNodes.insertElem(conflictTra->id);
			}
			for (auto i = 0; i< delBoxchangeTras.Num; ++i) {
				Traffic *tra = &traffics[delBoxchangeTras.data[i]];
				if (tra != conflictTra) {
					tra->nodes = tra->best_changeNodes;
				}
			}
			conflictTra->nodes = conflictTraOldNodes;
			waveGroups[original_w].deleteTra(conflictTra, conflictTras);
			waveGroups[original_w].afreshAdjMat(topo.adjMat);
			waveGroups[original_w].recordConflictTras(topo, conflictTras);
			make_layerMove(original_w, best_w, best_newTrasNodes, conflictTra);
			effectIterNum++;
			check_all(1);
		}
		else {
			bool change = false;
			conflictTra->nodes = conflictTraOldNodes;

			check_all(1);
			/*if (!change) {
				waveGroups[conflictTra->wave].addTra(conflictTra);
				waveGroups[conflictTra->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
				waveGroups[conflictTra->wave].recordConflictTras(topo, conflictTras);
				check_all(1);

			}*/
		}
	}

	void RWAsolver::layeredFind_move_m_best_original()
	{
		int non_tabu_cnt = 1;
		Traffic *conflictTra = getOneConflictTra();
		if (conflictTra == NULL) return;
		int original_w = conflictTra->wave;
		int confTraId = conflictTra->id;
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		int best_w = 0;

		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		int del_delt = delTraFromOriginalBox(conflictTra, delBoxchangeTras);
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		// del_delt为业务从原颜色盒子移除减少的冲突数
		int cnt = 1;
		int add_delt;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<ID> valuedWave;
		List<pair<ID, pair<int, int>>> bin_value;

		//List<pair<ID, int>> bin_value;
		valuedWave.reserve(WaveNum);
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
					delta[confTraId][new_w] = add_delt;
				}
				else {
					add_delt = delta[confTraId][new_w];
				}
				//int temp_delt = del_delt - add_delt;
				
				//bin_value.push_back(make_pair(new_w, make_pair(add_delt, leaveTime[conflictTra->id][new_w])));
				bin_value.push_back(make_pair(new_w, make_pair(add_delt, waveGroups[new_w].tras.size())));

				//bin_value.push_back(make_pair(new_w, add_delt));
			}
		}
		int m = env.mBest;
		//get_mBest_bin(bin_value, m);
		partial_sort(bin_value.begin(), bin_value.begin() + m, bin_value.end(), simple_compare_increase);
		best_delt = del_delt - bin_value[0].second.first;
		if (best_delt >= -2) {
			int temp_best_delt = MyIntMin;
			int cnt = 1;
			List<ID> conflitTra_bestNodes;
			for (int i = 0; i < m; i++) {
				int cur_w = bin_value[i].first;
				tempNewNodes.delAllElem();
				int cur_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[cur_w], tempNewNodes);
				if (cur_delt > temp_best_delt) {
					temp_best_delt = cur_delt;
					best_w = cur_w;
					cnt = 1;

					conflitTra_bestNodes = conflictTra->best_changeNodes;
					best_newTrasNodes.delAllElem();
					for (auto i = 0; i < tempNewNodes.Num; ++i) {
						best_newTrasNodes.insertElem(tempNewNodes.data[i]);
					}
				}
				else if (cur_delt == temp_best_delt) {
					cnt++;
					if (cntCentProbChoice(cnt)) {
					//if(waveGroups[cur_w].tras.size()> waveGroups[best_w].tras.size()){
						best_w = cur_w;
						conflitTra_bestNodes = conflictTra->best_changeNodes;

						best_newTrasNodes.delAllElem();
						for (auto i = 0; i < tempNewNodes.Num; ++i) {
							best_newTrasNodes.insertElem(tempNewNodes.data[i]);
						}
					}
				}
			}
			conflictTra->best_changeNodes = conflitTra_bestNodes;
			best_delt = temp_best_delt;
		}

		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb) //&& noBeterTimes >= 300
			)) {
			if (best_newTrasNodes.Num == 0) {
				dijkstraMinConflicts(waveGroups[best_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
				best_delt = del_delt - add_delt;
				conflictTra->best_changeNodes = conflictTra->nodes = temp_nodes;
				best_newTrasNodes.insertElem(conflictTra->id);
			}
			make_layerMove(original_w, best_w, best_newTrasNodes, conflictTra);
			effectIterNum++;
		}
		else {
			bool change = false;
			if (!change) {
				waveGroups[conflictTra->wave].addTra(conflictTra);
				waveGroups[conflictTra->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
				waveGroups[conflictTra->wave].recordConflictTras(topo, conflictTras);
				check_all(1);

			}
		}
	}
	void RWAsolver::layeredFind_move_coarsed_grained()
	{
		int non_tabu_cnt = 1;
		Traffic *conflictTra = getOneConflictTra();
		int original_w = conflictTra->wave;
		int confTraId = conflictTra->id;
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		int best_w = 0;

		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		int del_delt = delTraFromOriginalBox(conflictTra, delBoxchangeTras);
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		// del_delt为业务从原颜色盒子移除减少的冲突数
		int cnt = 1;
		int add_delt;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<ID> valuedWave;
		List<pair<ID, int>> bin_value;
		valuedWave.reserve(WaveNum);
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
					delta[confTraId][new_w] = add_delt;
				}
				else {
					add_delt = delta[confTraId][new_w];
				}
				int temp_delt = del_delt - add_delt;
				bin_value.push_back(make_pair(new_w, temp_delt));

				if (temp_delt > best_delt) {
					non_tabu_cnt = 1;
					best_delt = temp_delt;
					best_w = new_w;
				}
				else if (temp_delt > sBest_delt) {
					nonTabu_sBest_cnt = 1;
					sBest_delt = temp_delt;
					sBest_w = new_w;
				}
				else if (temp_delt == best_delt) {
					non_tabu_cnt++;
					if (cntCentProbChoice(non_tabu_cnt)) {
						best_w = new_w;
					}
				}
				else if (temp_delt == sBest_delt) {
					nonTabu_sBest_cnt++;
					if (cntCentProbChoice(nonTabu_sBest_cnt)) {
						sBest_w = new_w;
					}
				}
			}
		}
		best_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[best_w], tempNewNodes);
		
		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb))
			) {
			if (best_newTrasNodes.Num == 0) {
				dijkstraMinConflicts(waveGroups[best_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
				best_delt = del_delt - add_delt;
				conflictTra->best_changeNodes = conflictTra->nodes = temp_nodes;
				best_newTrasNodes.insertElem(conflictTra->id);
			}
			make_layerMove(original_w, best_w, best_newTrasNodes, conflictTra);
			effectIterNum++;
		}
		else {
			bool change = false;
			if (!change) {
				waveGroups[conflictTra->wave].addTra(conflictTra);
				waveGroups[conflictTra->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
				waveGroups[conflictTra->wave].recordConflictTras(topo, conflictTras);
				check_all(1);

			}
		}
	}
	void RWAsolver::layeredFind_move_all_by_dijkstra()
	{
		int non_tabu_cnt = 1;
		Traffic *conflictTra = getOneConflictTra();
		int original_w = conflictTra->wave;
		int confTraId = conflictTra->id;
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		int best_w = 0;

		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		List<ID> oldnodes = conflictTra->nodes;
		int del_delt = waveGroups[original_w].deleteTra(conflictTra, conflictTras);
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		// del_delt为业务从原颜色盒子移除减少的冲突数
		int cnt = 1;
		int add_delt;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<ID> valuedWave;
		List<pair<ID, int>> bin_value;
		valuedWave.reserve(WaveNum);
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {// 说明记录了评估值之后new_w盒子又发生了改变，需要重新记录
					recordIter[confTraId][new_w] = iteration;
					dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
					delta[confTraId][new_w] = add_delt;
				}
				else {
					add_delt = delta[confTraId][new_w];
				}
				int temp_delt = del_delt - add_delt;
				bin_value.push_back(make_pair(new_w, temp_delt));

				if (temp_delt > best_delt) {
					non_tabu_cnt = 1;
					best_delt = temp_delt;
					best_w = new_w;
				}
				else if (temp_delt > sBest_delt) {
					nonTabu_sBest_cnt = 1;
					sBest_delt = temp_delt;
					sBest_w = new_w;
				}
				else if (temp_delt == best_delt) {
					non_tabu_cnt++;
					if (cntCentProbChoice(non_tabu_cnt)) {
						best_w = new_w;
					}
				}
				else if (temp_delt == sBest_delt) {
					nonTabu_sBest_cnt++;
					if (cntCentProbChoice(nonTabu_sBest_cnt)) {
						sBest_w = new_w;
					}
				}
			}
		}
		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb))
			) {
			if (best_newTrasNodes.Num == 0) {
				dijkstraMinConflicts(waveGroups[best_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
				best_delt = del_delt - add_delt;
				conflictTra->best_changeNodes = conflictTra->nodes = temp_nodes;
				best_newTrasNodes.insertElem(conflictTra->id);
			}
			make_layerMove(original_w, best_w, best_newTrasNodes, conflictTra);
			effectIterNum++;
		}
		else {
			bool change = false;
			if (!change) {
				conflictTra->nodes = oldnodes;
				waveGroups[conflictTra->wave].addTra(conflictTra);
				//waveGroups[conflictTra->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
				waveGroups[conflictTra->wave].recordConflictTras(topo, conflictTras);
				check_all(1);

			}
		}
	}
	
	void RWAsolver::layeredFind_move_all_by_ejection()
	{
		int non_tabu_cnt = 1;
		Traffic *conflictTra = getOneConflictTra();
		int original_w = conflictTra->wave;
		int confTraId = conflictTra->id;
		best_delt = MyIntMin;// make_move 将conflictTra移动到最优颜色盒子中
		int best_w = 0;

		//将该冲突业务从原颜色盒子中删除，并统计冲突减少数，先直接删除
		int del_delt = delTraFromOriginalBox(conflictTra, delBoxchangeTras);
		best_newTrasNodes.delAllElem();
		tempNewNodes.delAllElem();
		// del_delt为业务从原颜色盒子移除减少的冲突数
		int cnt = 1;
		int add_delt;
		int sBest_delt = MyIntMin, nonTabu_sBest_cnt = 1, sBest_w = 0;

		List<ID> best_nodes;
		List<ID> temp_nodes;
		List<ID> valuedWave;
		if (iteration == 7) {
			cout << "test" << endl;
		}
		// 不带缓存
	/*	List<ID> conflictTra_bestNodes;
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {

				tempNewNodes.delAllElem();
				int cur_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[new_w], tempNewNodes);
				if (cur_delt > best_delt) {
					best_delt = cur_delt;
					best_w = new_w;
					cnt = 1;
					conflictTra_bestNodes = conflictTra->best_changeNodes;
					best_newTrasNodes.delAllElem();
					for (auto i = 0; i < tempNewNodes.Num; ++i) {
						best_newTrasNodes.insertElem(tempNewNodes.data[i]);
					}
				}
				else if (cur_delt == best_delt) {
					cnt++;
					if (cntCentProbChoice(cnt)) {
						best_w = new_w;
						best_newTrasNodes.delAllElem();
						conflictTra_bestNodes = conflictTra->best_changeNodes;

						for (auto i = 0; i < tempNewNodes.Num; ++i) {
							best_newTrasNodes.insertElem(tempNewNodes.data[i]);
						}
					}
				}
			}
		}
		
		conflictTra->best_changeNodes = conflictTra_bestNodes;*/
		
		// 带缓存
		for (auto new_w = 0; new_w < WaveNum; ++new_w) {
			if (new_w != original_w) {
				if (recordIter[confTraId][new_w] <= groupChangedIter[new_w]) {
					recordIter[confTraId][new_w] = iteration;
					tempNewNodes.delAllElem();
					add_delt = tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[new_w], tempNewNodes);
					delta[confTraId][new_w] = add_delt;
				}
				else {
					add_delt = delta[confTraId][new_w];
				}
				int cur_delt = del_delt - add_delt;
				if (cur_delt > best_delt) {
					best_delt = cur_delt;
					best_w = new_w;
					cnt = 1;
				}
				else if (cur_delt == best_delt) {
					cnt++;
					if (cntCentProbChoice(cnt)) {
						best_w = new_w;
					}
				}
			}
		}
		best_delt = del_delt - tryAddTraToNewBoxByEjectionChain_Grasp(conflictTra, waveGroups[best_w], best_newTrasNodes);

	
		// make_move 将conflictTra移动到最优颜色盒子中
		if (best_delt >= 0 || (best_delt == -1 && cntCentProbChoice(env.worseSolProb))
			) {
			if (best_newTrasNodes.Num == 0) {
				dijkstraMinConflicts(waveGroups[best_w].adjMatTraNums, conflictTra, add_delt, temp_nodes);
				best_delt = del_delt - add_delt;
				conflictTra->best_changeNodes = conflictTra->nodes = temp_nodes;
				best_newTrasNodes.insertElem(conflictTra->id);
			}
			make_layerMove(original_w, best_w, best_newTrasNodes, conflictTra);
			check_all(1);
			effectIterNum++;
		}
		else {
			bool change = false;
			if (!change) {
				waveGroups[conflictTra->wave].addTra(conflictTra);
				waveGroups[conflictTra->wave].recoverFromOldNodes(traffics, delBoxchangeTras);
				waveGroups[conflictTra->wave].recordConflictTras(topo, conflictTras);
				check_all(1);

			}
		}

	}
	void RWAsolver::make_layerMove(int original_w, int best_w, TwoWayList &best_newTrasNodes, Traffic *conflictTra) {
		groupChangedIter[original_w] = iteration;
		groupChangedIter[best_w] = iteration;
		leaveTime[conflictTra->id][original_w] = iteration;
		for (auto i = 0; i< best_newTrasNodes.Num; ++i) {
			Traffic *tra = &traffics[best_newTrasNodes.data[i]];
			tra->nodes = tra->best_changeNodes;
		}
		waveGroups[best_w].addTra(conflictTra);
		conflictTra->wave = best_w;
		waveGroups[best_w].afreshAdjMat(topo.adjMat);
		waveGroups[best_w].recordConflictTras(topo, conflictTras);

		conflictsNum -= best_delt;
	}
	void RWAsolver::make_exchangeMove(int wA, int wB, TwoWayList & newTrasNodesA, TwoWayList & newTrasNodesB)
	{
		groupChangedIter[wA] = iteration;
		groupChangedIter[wB] = iteration;
		for (auto i = 0; i < newTrasNodesA.Num; i++){
			Traffic *tra = &traffics[newTrasNodesA.data[i]];
			tra->nodes = tra->best_changeNodes;
		}
		for (auto i = 0; i < newTrasNodesB.Num; i++) {
			Traffic *tra = &traffics[newTrasNodesB.data[i]];
			tra->nodes = tra->best_changeNodes;
		}
		waveGroups[wA].afreshAdjMat(topo.adjMat);
		waveGroups[wA].recordConflictTras(topo, conflictTras);
		waveGroups[wB].afreshAdjMat(topo.adjMat);
		waveGroups[wB].recordConflictTras(topo, conflictTras);

	}
	void RWAsolver::find_move(Move &move)
	{
		//int tabu_cnt = 1, non_tabu_cnt = 1;
		//int tabu_move_delt = INT_MIN, non_tabu_move_delt = INT_MIN;
		//Move tabu_move, non_tabu_move;

		//int conflictTraNum = conflictTras.size();
		//auto iter = conflictTras.begin();
		//int index = rand() % conflictTraNum;//从有冲突的业务中随机选择一个
		//for (auto i = 0; i < index; ++i) {
		//	iter++;
		//}
		//Traffic *conflictTra = (*iter);
		//int original_w = conflictTra->wave;
		//nonTabu_best_delt = INT_MIN;

		//int del_delt = waveGroups[original_w].tryDelTra(conflictTra); //尝试删除该业务对总冲突数减少的数量
		//int cnt = 1;

		//for (auto new_w = 0; new_w < WaveNum; ++new_w) {
		//	if (new_w != original_w) {// 尝试将业务从波长original_w更换到波长new_w
		//		int add_delt = 0;
		//		{/// 使用模型进行评估
		//			/*waveGroups[new_w].tras.push_back(conflictTra);
		//			Model model(waveGroups[new_w].tras, nodeNum, topo, grb_env,Linear);
		//			waveGroups[new_w].tras.pop_back();
		//			add_delt = model.traNumShouldRemove - waveGroups[new_w].conflictsOnAllLinks; */
		//			//新增了业务之后的总冲突数变化
		//		}
		//		{/// 使用dijkstra进行近似评估
		//			dijkstraMinConflicts(waveGroups[new_w].adjMatTraNums, conflictTra, add_delt);
		//		}
		//		//add_delt >= 0
		//		if (del_delt - add_delt > nonTabu_best_delt) { // nonTabu_best_delt越大说明下降的越多
		//			nonTabu_best_delt = del_delt - add_delt;
		//			move.tra = conflictTra;
		//			move.old_w = original_w;
		//			move.new_w = new_w;
		//			cnt = 1;
		//		}
		//		else if (del_delt - add_delt == nonTabu_best_delt) {
		//			cnt++;
		//			if (cntCentProbChoice(cnt)) {
		//				move.tra = conflictTra;
		//				move.old_w = original_w;
		//				move.new_w = new_w;
		//				//cout << "Same delt" << endl;
		//			}
		//		}
		//	}
		//}
		

	}
	void RWAsolver::solveModelAndGetAnswer(WaveGroup &waveGroup)
	{
		//Model model(waveGroup.tras, nodeNum, topo, grb_env,Interger); //使用模型最小化同一个波长组的业务的冲突
		//model.getAnswer(waveGroup.tras, topo);
		//conflictsNum -= (waveGroup.conflictsOnAllLinks - model.traNumShouldRemove);
		//waveGroup.conflictsOnAllLinks = model.traNumShouldRemove;

	}

	
	
	void RWAsolver::make_moveByModel(Move &move)
	{
		int add_delt=0;
		waveGroups[move.old_w].deleteTra(move.tra, conflictTras);
		solveModelAndGetAnswer(waveGroups[move.old_w]);
		waveGroups[move.old_w].afreshAdjMat(topo.adjMat);
		waveGroups[move.old_w].recordConflictTras(topo, conflictTras);

		move.tra->wave = move.new_w;
		waveGroups[move.new_w].addTra(move.tra);
		solveModelAndGetAnswer(waveGroups[move.new_w]);
		waveGroups[move.new_w].afreshAdjMat(topo.adjMat);
		waveGroups[move.new_w].recordConflictTras(topo, conflictTras);


		//printWaveGroups();

	}

	void RWAsolver::make_moveByDij(Move & move)
	{
		int add_delt, del_delt;
		del_delt = waveGroups[move.old_w].tryDelTra(move.tra);
		waveGroups[move.old_w].conflictsOnAllLinks -= del_delt;
#ifdef DEBUG_CHECK_MOVE_DIJ
		int temp_cnt = 0;
		for (auto j = 0; j < move.tra->nodes.size() - 1; ++j) {
			ID node3 = move.tra->nodes[j], node4 = move.tra->nodes[j + 1];
			bool find = false;
			for (auto t = waveGroups[move.old_w].tras.begin(); (t != waveGroups[move.old_w].tras.end()) && !find; ++t) {
				if (*t != move.tra) {
					for (auto i = 0; i < (*t)->nodes.size() - 1; ++i) {
						ID node1 = (*t)->nodes[i], node2 = (*t)->nodes[i + 1];

						if (node1 == node3 && node2 == node4) {
							find = true;
							temp_cnt++;
							break;
						}
					}
				}
			}
		}
		if (temp_cnt != del_delt) {
			Log(LogSwitch::FY) << "make_moveByDij1 error" << endl;

		}
		if (waveGroups[move.old_w].conflictsOnAllLinks < 0) {
			Log(LogSwitch::FY) << "make_moveByDij2 error ";
			Log(LogSwitch::FY) << move.tra->id <<" "<<move.old_w<<" "<<move.new_w << endl;
		}
#endif // DEBUG_CHECK_MOVE_DIJ
		waveGroups[move.old_w].deleteTra(move.tra, conflictTras);
		waveGroups[move.old_w].recordConflictTras(topo, conflictTras);

		move.tra->wave = move.new_w;
		
		dijkstraMinConflicts(waveGroups[move.new_w].adjMatTraNums, move.tra, add_delt, move.tra->nodes);
#ifdef DEBUG_CHECK_MOVE_DIJ
		temp_cnt = 0;
		for (auto j = 0; j < move.tra->nodes.size() - 1; ++j) {
			ID node3 = move.tra->nodes[j], node4 = move.tra->nodes[j + 1];
			bool find = false;
			for (auto t = waveGroups[move.new_w].tras.begin(); (t != waveGroups[move.new_w].tras.end()) && !find; ++t) {
				if (*t != move.tra) {
					for (auto i = 0; i < (*t)->nodes.size() - 1; ++i) {
						ID node1 = (*t)->nodes[i], node2 = (*t)->nodes[i + 1];

						if (node1 == node3 && node2 == node4) {
							find = true;
							temp_cnt++;
							break;
						}
					}
				}
			}
		}
		if (temp_cnt != add_delt) {
			Log(LogSwitch::FY) << "make_moveByDij3 error" << endl;

		}
#endif // DEBUG_CHECK_MOVE_DIJ
		waveGroups[move.new_w].conflictsOnAllLinks += add_delt;
		waveGroups[move.new_w].addTra(move.tra);
		waveGroups[move.new_w].recordConflictTras(topo, conflictTras);
		conflictsNum = conflictsNum - del_delt + add_delt;
	}

	void RWAsolver::printWaveGroup(int w)
	{
		Log(LogSwitch::FY) << "--------------------- wave = " << w << " ------------------------" << endl;
		for (auto i = 0; i < nodeNum; ++i) {
			for (auto j = 0; j < nodeNum; ++j) {
				if (waveGroups[w].adjMat[i][j] == MaxDistance)
					Log(LogSwitch::FY) << 9 << "  ";
				else
					Log(LogSwitch::FY) << waveGroups[w].adjMat[i][j] << "  ";
			}
			Log(LogSwitch::FY) << endl;
		}
		for (auto i = 0; i < nodeNum; ++i) {
			for (auto j = 0; j < nodeNum; ++j) {
				Log(LogSwitch::FY) << waveGroups[w].adjMatTraNums[i][j] << "  ";
			}
			Log(LogSwitch::FY) << endl;
		}
		Log(LogSwitch::FY) << "---------------------------------------------" << endl;
	}
	

	void RWAsolver::printWaveGroups()
	{
		for (auto w = 0; w < WaveNum; ++w) {
			printWaveGroup(w);
		}
	}

	//void RWAsolver::Model::addCapacityConstraints(Topo &topo,int waveNum)
	//{
	//	
	//	for (auto j = 0; j < nodeNum; ++j) {
	//		for (auto k = 0; k < topo.linkNodes[j].size(); ++k) {
	//			int v = topo.linkNodes[j][k];
	//			GRBLinExpr expr = 0;
	//			for (auto i = 0; i < groupTrasNum; ++i) {
	//				expr += trafficOnLink[i][j][v];
	//			}
	//			model->addConstr(expr <= waveNum, "f-" + to_string(j) + "-" + to_string(v));
	//		}
	//	}
	//}

	//void RWAsolver::Model::addFlowVar(Topo &topo, ModelType type)
	//{
	//	trafficOnLink.resize(groupTrasNum);
	//	for (auto i = 0; i < groupTrasNum; ++i) {
	//		trafficOnLink[i].resize(nodeNum);
	//		for (auto j = 0; j < nodeNum; ++j) {
	//			trafficOnLink[i][j].resize(nodeNum);
	//		}
	//	}
	//	for (auto i = 0; i < groupTrasNum; ++i) {
	//		for (auto j = 0; j < nodeNum; ++j) {
	//			for (auto k = 0; k < topo.linkNodes[j].size(); ++k) {
	//				int v = topo.linkNodes[j][k];
	//				if (type == Interger) {
	//					trafficOnLink[i][j][v] = model->addVar(0.0, 1.0, 0.0, GRB_BINARY, "f-" +
	//						std::to_string(i) + "-" + std::to_string(j) + "-" + std::to_string(v));
	//				}
	//				else {
	//					trafficOnLink[i][j][v] = model->addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, "f-" +
	//						std::to_string(i) + "-" + std::to_string(j) + "-" + std::to_string(v));
	//				}
	//			}
	//		}
	//	}
	//}

	//void RWAsolver::Model::addFlowConstraints(List<Traffic*>& groupTras, Topo &topo)
	//{

	//	for (auto i = 0; i < groupTrasNum; ++i) {
	//		int src = groupTras[i]->src, dst = groupTras[i]->dst;
	//		for (auto u = 0; u < nodeNum; ++u) {
	//			GRBLinExpr expr1 = 0, expr2 = 0;
	//			if (topo.linkNodes[u].size() > 0) {
	//				for (auto j = 0; j < topo.linkNodes[u].size(); ++j) {
	//					ID m = topo.linkNodes[u][j];
	//					expr1 += trafficOnLink[i][u][m];

	//					expr2 += trafficOnLink[i][m][u];
	//				}
	//				model->addConstr(expr1 <= 1, "O-" + std::to_string(i) + "-" + std::to_string(u));//Out degree constraint
	//				if (u != src && u != dst) {
	//					model->addConstr(expr1 - expr2 == 0, "F-" + std::to_string(i) + "-" + std::to_string(u) + "-");
	//				}
	//				else if (u == src) {
	//					model->addConstr(expr1 - expr2 == 1, "S-" + std::to_string(i) + "-" + std::to_string(u) + "-");

	//				}
	//				else if (u == dst) {
	//					model->addConstr(expr2 - expr1 == 1, "D-" + std::to_string(i) + "-" + std::to_string(u) + "-");

	//				}
	//			}

	//		}
	//	}
	//}

	//void RWAsolver::Model::minTraNumShouldRemove( Topo &topo)
	//{
	//	shouldRemoveTra.resize(nodeNum);
	//	auxVar.resize(nodeNum);
	//	for (auto i = 0; i < nodeNum; ++i) {
	//		shouldRemoveTra[i].resize(nodeNum);
	//		auxVar[i].resize(nodeNum);
	//	}
	//	for (auto i = 0; i < nodeNum; ++i) {
	//		for (auto j = 0; j < topo.linkNodes[i].size(); ++j) {
	//			int v = topo.linkNodes[i][j];
	//			shouldRemoveTra[i][v] = model->addVar(-1.0, groupTrasNum, 0.0, GRB_INTEGER,
	//				"x-" + std::to_string(i) + "-" + std::to_string(v));

	//			auxVar[i][v] = model->addVar(0.0, 1.0, 0.0, GRB_BINARY,
	//				"b-" + std::to_string(i) + "-" + std::to_string(v));
	//		}

	//	}
	//	z = model->addVar(0.0, groupTrasNum*nodeNum, 0.0, GRB_INTEGER, "z");

	//	GRBLinExpr expr = 0;
	//	int M = groupTrasNum + 2;
	//	for (auto m = 0; m < nodeNum; ++m) {

	//		for (auto j = 0; j < topo.linkNodes[m].size(); ++j) {
	//			ID 	n = topo.linkNodes[m][j];
	//			GRBLinExpr expr2 = 0;
	//			for (auto t = 0; t < groupTrasNum; ++t) {
	//				expr2 += trafficOnLink[t][m][n];
	//			}
	//			model->addConstr(shouldRemoveTra[m][n] >= -1 + expr2, "max1-" + std::to_string(m) + "-" + std::to_string(n));
	//			model->addConstr(shouldRemoveTra[m][n] >= 0, "max2-" + std::to_string(m) + "-" + std::to_string(n));
	//			model->addConstr(shouldRemoveTra[m][n] <= -1 + expr2 + M * auxVar[m][n], "max3-" + std::to_string(m) + "-" + std::to_string(n));
	//			model->addConstr(shouldRemoveTra[m][n] <= M * (1 - auxVar[m][n]), "max4-" + std::to_string(m) + "-" + std::to_string(n));

	//			expr += shouldRemoveTra[m][n];
	//			
	//		}
	//	}
	//	model->addConstr(z == expr, "max5");

	//	GRBLinExpr obj = z;
	//	model->setObjective(obj, GRB_MINIMIZE);
	//	model->optimize();
	//}

	//void RWAsolver::Model::minFlow(Topo & topo)
	//{
	//	GRBLinExpr expr1 = 0;
	//	for (auto m = 0; m < nodeNum; ++m) {
	//		for (auto j = 0; j < topo.linkNodes[m].size(); ++j) {
	//			ID 	n = topo.linkNodes[m][j];
	//			for (auto t = 0; t < groupTrasNum; ++t) {
	//				expr1 += trafficOnLink[t][m][n];
	//			}
	//		}
	//	}
	//	model->setObjective(expr1, GRB_MINIMIZE);
	//	model->optimize();
	//}

	//void RWAsolver::Model::setLinearObj(Topo & topo, ModelType type)
	//{
	//	
	//	for (auto m = 0; m < nodeNum; ++m) {
	//		for (auto j = 0; j < topo.linkNodes[m].size(); ++j) {
	//			ID 	n = topo.linkNodes[m][j];
	//			GRBLinExpr expr1 = 0;
	//			for (auto t = 0; t < groupTrasNum; ++t) {
	//				expr1 += trafficOnLink[t][m][n];
	//			}
	//			GRBVar delta_mn;
	//			if (type == Interger) {
	//				delta_mn  = model->addVar(0, groupTrasNum, 0, GRB_INTEGER, "Delta-" + std::to_string(m) + "-" + std::to_string(n));

	//			}
	//			else {
	//				delta_mn = model->addVar(0, groupTrasNum, 0, GRB_CONTINUOUS, "Delta-" + std::to_string(m) + "-" + std::to_string(n));
	//			}
	//			expr1 = expr1 - delta_mn;
	//			deltas.push_back(delta_mn);
	//			model->addConstr(expr1 <= 1, "delta<=1");
	//		}
	//	}
	//	GRBLinExpr obj = 0;
	//	for (auto i = 0; i < deltas.size(); ++i) {
	//		obj += deltas[i];
	//	}
	//	model->setObjective(obj, GRB_MINIMIZE);
	//	model->optimize();
	//}

	//void RWAsolver::Model::getAnswer( List<Traffic*>& groupTras, Topo &topo)
	//{
	//	//model->write("model.lp");
	//	//model->write("out.sol");

	//	for (auto i = 0; i < groupTrasNum; ++i) {
	//		ID src = groupTras[i]->src, dst = groupTras[i]->dst;
	//		ID cur = src, next;
	//		groupTras[i]->nodes.clear();
	//		while (cur != dst) {
	//			int cnt = 0;
	//			groupTras[i]->nodes.push_back(cur);
	//			for (auto j = 0; j < topo.linkNodes[cur].size(); ++j) {
	//				ID temp = topo.linkNodes[cur][j];
	//				if (trafficOnLink[i][cur][temp].get(GRB_DoubleAttr_X) > 0.0) {
	//					cnt++;
	//					if (cnt >= 2) {
	//						std::cout << "Outdegree Error\n";
	//						exit(-1);
	//					}
	//					next = temp;
	//					break;
	//				}
	//			}
	//			cur = next;
	//		}
	//		groupTras[i]->nodes.push_back(cur);

	//	}
	//}

}
