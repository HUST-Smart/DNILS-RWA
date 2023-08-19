#pragma once
#include<vector>
#include<iostream>
#include<sstream>
#include<fstream>
#include <thread>
#include <queue>
#include<map>
#include "../Simulator/ThreadPool.h"
//#include "gurobi_c++.h"
#include"Common.h"
#include"Utility.h"
#include"LogSwitch.h"
#include "Problem.h"
namespace szx {
	struct BestSolution {
		List<ID> bestNodes;
		Wave wave;
	};
	struct Traffic {
	public:
		ID id;
		ID src;
		ID dst;
		List<ID> nodes; // 业务的路径
		List<ID> shorestNodes; // 业务在不考虑波长分配时的最短路径
		List<ID> oldNodes; // 业务的原始路径
		List<ID> best_changeNodes;
		BestSolution bestSol;
		Wave wave; // 业务使用的波长
		Traffic(){}
		Traffic(int id, int src, int dst) :id(id), src(src), dst(dst) {}
	};
	struct TwoWayList {
	public:
		List<ID> data;
		List<ID> indexArr;
		int Num;
		TwoWayList &operator=(const TwoWayList &tl) {
			data = tl.data;
			indexArr = tl.indexArr;
			Num = tl.Num;
			return *this;
		}
		/*TwoWayList &operator=(const TwoWayList &tl) {
			if (data.size() != tl.data.size()) {
				data = tl.data;
				indexArr = tl.indexArr;
				Num = tl.Num;
			}
			else {
				delAllElem();
				for (auto i = 0; i < Num; i++) {
					insertElem(tl.data[i]);
				}
			}
			
			return *this;
		}*/
		ID operator[](const int i) {
			return data[i];
		}
		void initSpace(int size) {
			data.clear();
			indexArr.clear();
			data.resize(size, -1);
			indexArr.resize(size, -1);
			Num = 0;
		}
		bool isElemExisit(ID elem) {
			return indexArr[elem] != -1;
		}
		void insertElem(ID elem) {
			if (isElemExisit(elem)) return;
			data[Num] = elem;
			indexArr[elem] = Num++;
		}
		void delElem(ID elem) {
			if (!isElemExisit(elem)) return;

			int index = indexArr[elem];
			data[index] = data[--Num];
			indexArr[data[index]] = index;
			indexArr[elem] = -1;
		}
		void delAllElem() {
			for (auto i = 0; i < Num; ++i) {
				indexArr[data[i]] = -1;
			}
			Num = 0;
		}
	};

	
	template<typename T>
	auto select_random(const T &t, size_t n) {
		auto iter = std::begin(t);
		std::advance(iter, n);
		return iter;
	}
	class RWAsolver {
#pragma region Type
	public:
		TwoWayList conflictTras; // 保存所有有冲突的业务
		TwoWayList bestConflicTras; // 最优解中有冲突的业务

		struct Cli {
			static constexpr int MaxArgLen = 256;
			static constexpr int MaxArgNum = 32;

			static String InstancePathOption() { return "-p"; }
			static String SolutionPathOption() { return "-o"; }
			static String CurrentBestOption() { return "-cb"; }
			static String RandSeedOption() { return "-s"; }
			static String TimeoutOption() { return "-t"; }
			static String MaxIterOption() { return "-i"; }
			static String JobNumOption() { return "-j"; }
			static String RunIdOption() { return "-rid"; }
			static String EnvironmentPathOption() { return "-env"; }
			static String ConfigPathOption() { return "-cfg"; }
			static String LogPathOption() { return "-log"; }
			static String PerturbInterval() { return "-pi"; }
			static String PerturbTprob() { return "-pt"; }
			static String PerturbNum() { return "-pn"; }
			static String WorseSolProb() { return "-wsp"; }
			static String SwithScale() { return "-ss"; }
			static String MBest() { return "-mb"; }
			static String AuthorNameSwitch() { return "-name"; }
			static String HelpSwitch() { return "-h"; }

			static String AuthorName() { return "szx"; }
			static String HelpInfo() {
				return "Pattern (args can be in any order):\n"
					"  exe (-p path) (-o path) [-s int] [-t seconds] [-name]\n"
					"      [-iter int] [-j int] [-id string] [-h]\n"
					"      [-env path] [-cfg path] [-log path]\n"
					"Switches:\n"
					"  -name  return the identifier of the authors.\n"
					"  -h     print help information.\n"
					"Options:\n"
					"  -p     input instance file path.\n"
					"  -o     output solution file path.\n"
					"  -s     rand seed for the solver.\n"
					"  -t     max running time of the solver.\n"
					"  -i     max iteration of the solver.\n"
					"  -j     max number of working solvers at the same time.\n"
					"  -rid   distinguish different runs in log file and output.\n"
					"  -env   environment file path.\n"
					"  -cfg   configuration file path.\n"
					"  -log   activate logging and specify log file path.\n"
					"Note:\n"
					"  0. in pattern, () is non-optional group, [] is optional group\n"
					"     when -env option is not given.\n"
					"  1. an environment file contains information of all options.\n"
					"     explicit options get higher priority than this.\n"
					"  2. reaching either timeout or iter will stop the solver.\n"
					"     if you specify neither of them, the solver will be running\n"
					"     for a long time. so you should set at least one of them.\n"
					"  3. the solver will still try to generate an initial solution\n"
					"     even if the timeout or max iteration is 0. but the solution\n"
					"     is not guaranteed to be feasible.\n";
			}

			// a dummy main function.
			static int run(int argc, char *argv[]);
		};
		struct Configuration {
			enum Algorithm { Greedy, TreeSearch, DynamicProgramming, LocalSearch, Genetic, MathematicallProgramming };


			Configuration() {}


			String toBriefStr() const {
				String threadNum(std::to_string(threadNumPerWorker));
				std::ostringstream oss;
				oss << "alg=" << alg
					<< ";job=" << threadNum;
				return oss.str();
			}

			
			Algorithm alg = Configuration::Algorithm::Greedy; // OPTIMIZE[szx][3]: make it a list to specify a series of algorithms to be used by each threads in sequence.
			int threadNumPerWorker = (std::min)(1, static_cast<int>(std::thread::hardware_concurrency()));
		};

		// describe the requirements to the input and output data interface.
		struct Environment {
			static constexpr int DefaultTimeout = (1 << 30);
			static constexpr int DefaultMaxIter = (1 << 30);
			static constexpr int DefaultJobNum = 0;
			// preserved time for IO in the total given time.
			static constexpr int SaveSolutionTimeInMillisecond = 1000;

			static constexpr Duration RapidModeTimeoutThreshold = 600 * static_cast<Duration>(Timer::MillisecondsPerSecond);

			static String DefaultInstanceDir() { return "Instance/"; }
			static String DefaultSolutionDir() { return "Solution/"; }
			static String DefaultVisualizationDir() { return "Visualization/"; }
			static String DefaultEnvPath() { return "env.csv"; }
			static String DefaultCfgPath() { return "cfg.csv"; }
			static String DefaultLogPath() { return "log.csv"; }

			Environment(const String &instancePath, const String &solutionPath,
				int randomSeed = Random::generateSeed(), double timeoutInSecond = DefaultTimeout,
				Iteration maxIteration = DefaultMaxIter, int jobNumber = DefaultJobNum, String runId = "",
				const String &cfgFilePath = DefaultCfgPath(), const String &logFilePath = DefaultLogPath())
				: instPath(instancePath), slnPath(solutionPath), randSeed(randomSeed),
				msTimeout(static_cast<Duration>(timeoutInSecond * Timer::MillisecondsPerSecond)), maxIter(maxIteration),
				jobNum(jobNumber), rid(runId), cfgPath(cfgFilePath), logPath(logFilePath), localTime(Timer::getTightLocalTime()) {}
			Environment() : Environment("", "") {}

			void load(const Map<String, char*> &optionMap);
			void load(const String &filePath);
			void loadWithoutCalibrate(const String &filePath);
			void save(const String &filePath) const;

			void calibrate(); // adjust job number and timeout to fit the platform.

			String solutionPathWithTime() const { return slnPath + "." + localTime; }

			String visualizPath() const { return DefaultVisualizationDir() + friendlyInstName() + "." + localTime + ".html"; }
			template<typename T>
			String visualizPath(const T &msg) const { return DefaultVisualizationDir() + friendlyInstName() + "." + localTime + "." + std::to_string(msg) + ".html"; }
			String friendlyInstName() const { // friendly to file system (without special char).
				auto pos = instPath.find_last_of('/');
				return (pos == String::npos) ? instPath : instPath.substr(pos + 1);
			}
			String friendlyLocalTime() const { // friendly to human.
				return localTime.substr(0, 4) + "-" + localTime.substr(4, 2) + "-" + localTime.substr(6, 2)
					+ "_" + localTime.substr(8, 2) + ":" + localTime.substr(10, 2) + ":" + localTime.substr(12, 2);
			}

			// essential information.
			String instPath;
			String insName;
			String slnPath;
			int currentBestValue;
			int randSeed;
			int perturbInterval;
			int perturbTprob;
			int perturbNum;
			int worseSolProb;
			int swithScale;
			int mBest;
			Duration msTimeout;

			// optional information. highly recommended to set in benchmark.
			Iteration maxIter;
			int jobNum; // number of solvers working at the same time.
			String rid; // the id of each run.
			String cfgPath;
			String logPath;

			// auto-generated data.
			String localTime;
		};
		struct Solution : public Problem::Output { // cutting patterns.
			Solution(RWAsolver *pSolver = nullptr) : solver(pSolver) {}

			RWAsolver *solver;
		};

		struct Topo {
		public:
			List<List<ID>> linkNodes; // linkNodes[i]为结点i连接的所有的结点集合
			List<List<ID>> nextID; // nextID[i][j]为i到j的最短路径中i的下一个结点
			List<List<ID>> adjLinkMat; // adjLinkMat[i][j]为从节点i到结点j的链路ID，ID从1开始编号
			Arr2D<Length> adjMat;
			int link_num;
			Topo() {}
		};
		
		
		struct WaveGroup {
		public:
			List<Traffic *> tras;
			List<List<int>> adjMat; // 该颜色盒子的网络拓扑
			List<List<int>> adjMatTraNums; // adjMatTraNums[i][j]记录了(i,j)这条边上的业务数
			TwoWayList groupConflicTras;
			int conflictsOnAllLinks; // 所有边的冲突数之和
		public:
			WaveGroup(Arr2D<Length> &originalAdjMat,int trafficNum) {
				int nodeNum = originalAdjMat.size1();
				adjMat.resize(nodeNum);
				adjMatTraNums.resize(nodeNum);
				for (auto i = 0; i < nodeNum; ++i) {
					adjMat[i].resize(nodeNum);
					adjMatTraNums[i].resize(nodeNum, 0);
				}
				conflictsOnAllLinks = 0;
				for (auto i = 0; i < nodeNum; ++i) {
					for (auto j = 0; j < nodeNum; ++j) {
						adjMat[i][j] = originalAdjMat.at(i, j);
					}
				}
				groupConflicTras.initSpace(trafficNum);
			}
			explicit WaveGroup(const WaveGroup& group) {
				tras = group.tras;
				adjMat = group.adjMat;
				adjMatTraNums = group.adjMatTraNums;
				groupConflicTras = group.groupConflicTras;
				conflictsOnAllLinks = group.conflictsOnAllLinks;
			}
			WaveGroup &operator= (const WaveGroup &group) {
				tras = group.tras;
				adjMat = group.adjMat;
				adjMatTraNums = group.adjMatTraNums;
				groupConflicTras = group.groupConflicTras;
				conflictsOnAllLinks = group.conflictsOnAllLinks;
				return *this;
			}
			void recordOldNodes() {
				for (auto t = 0; t < tras.size(); ++t) {
					tras[t]->oldNodes = tras[t]->nodes;
				}
			}
			void delTraNodes(Traffic *tra) {
				for (auto i = 0; i < tra->nodes.size() - 1; ++i) {
					ID node1 = tra->nodes[i], node2 = tra->nodes[i + 1];
					if (adjMatTraNums[node1][node2] == 0) {
						Log(LogSwitch::FY) << "adjMatTraNums error in deleteTra" << std::endl;
						exit(-2);
					}
					if (adjMatTraNums[node1][node2] > 1)conflictsOnAllLinks--;
					adjMatTraNums[node1][node2]--;
					if (adjMatTraNums[node1][node2] == 0) {
						adjMat[node1][node2] = 1;
					}
				}
			}
			void addTraNodes(Traffic *tra) {
				for (auto i = 0; i < tra->nodes.size() - 1; ++i) {
					ID node1 = tra->nodes[i], node2 = tra->nodes[i + 1];
					adjMat[node1][node2] = MaxDistance;
					adjMatTraNums[node1][node2] ++;
					if (adjMatTraNums[node1][node2] > 1) conflictsOnAllLinks++;
				}
			}
			void recoverFromOldNodes(Topo &topo, Map<Traffic*, List<ID>>& newTrasNodes, Traffic *excludeTra = NULL) {
				for (auto iter = newTrasNodes.begin(); iter != newTrasNodes.end(); ++iter) {// 遍历该盒子中改变了路径的业务
					Traffic *tra = (*iter).first;
					if (tra == excludeTra) continue;
					delTraNodes(tra);
					tra->nodes = tra->oldNodes; //恢复成原来的路径
				}
				for (auto iter = newTrasNodes.begin(); iter != newTrasNodes.end(); ++iter) {// 遍历该盒子中改变了路径的业务
					Traffic *tra = (*iter).first;
					if (tra == excludeTra) continue;
					addTraNodes(tra);
				}
			}
			void recoverFromOldNodes(List<Traffic> &traffics, TwoWayList &delBoxchangeTras, Traffic *excludeTra = NULL) {
				for (auto i=0; i<delBoxchangeTras.Num; ++i) {// 遍历该盒子中改变了路径的业务
					Traffic *tra = &traffics[delBoxchangeTras.data[i]];
					if (tra == excludeTra) continue;
					delTraNodes(tra);
					tra->nodes = tra->oldNodes; //恢复成原来的路径
				}
				for (auto i = 0; i < delBoxchangeTras.Num; ++i) {// 遍历该盒子中改变了路径的业务
					Traffic *tra = &traffics[delBoxchangeTras.data[i]];
					if (tra == excludeTra) continue;
					addTraNodes(tra);
				}
			}
			void recordConflictTras(Topo &topo, TwoWayList & conflictTras) {
				for (auto i = 0; i < tras.size(); ++i) {
					bool overlap = false;
					for (auto j = 0; j < tras[i]->nodes.size() - 1; ++j) {
						ID node1 = tras[i]->nodes[j], node2 = tras[i]->nodes[j + 1];
						if (adjMatTraNums[node1][node2] > 1) {
							groupConflicTras.insertElem(tras[i]->id);
							conflictTras.insertElem(tras[i]->id);
							overlap = true;
							break;
						}
					}
					if (!overlap) {
						groupConflicTras.delElem(tras[i]->id);
						conflictTras.delElem(tras[i]->id);
					}
				}
			}
			void recordConflictTrasNotChangeGlobal(Topo &topo) {
				for (auto i = 0; i < tras.size(); ++i) {
					bool overlap = false;
					for (auto j = 0; j < tras[i]->nodes.size() - 1; ++j) {
						ID node1 = tras[i]->nodes[j], node2 = tras[i]->nodes[j + 1];
						if (adjMatTraNums[node1][node2] > 1) {
							groupConflicTras.insertElem(tras[i]->id);
							overlap = true;
							break;
						}
					}
					if (!overlap) {
						groupConflicTras.delElem(tras[i]->id);
					}
				}
			}
			void afreshAdjMat(Arr2D<Length> &originalAdjMat) {
				int nodeNum = originalAdjMat.size1();
				for (auto i = 0; i < nodeNum; ++i) {
					for (auto j = 0; j < nodeNum; ++j) {
						adjMat[i][j] = originalAdjMat.at(i, j);
						adjMatTraNums[i][j] = 0;
					}
				}
				conflictsOnAllLinks = 0;
				for (auto t = 0; t < tras.size(); ++t) {
					Traffic * tra = tras[t];
					for (auto i = 0; i < tra->nodes.size() - 1; ++i) {
						ID node1 = tra->nodes[i], node2 = tra->nodes[i + 1];
						adjMat[node1][node2] = MaxDistance;
						adjMatTraNums[node1][node2] ++;
						if (adjMatTraNums[node1][node2] > 1) {
							conflictsOnAllLinks++;
						}
					}
				}
			}
			void printTras() {
				for (auto t = 0; t < tras.size(); ++t) {
					Log(LogSwitch::FY) << tras[t]->id << ":";
					for (auto i = 0; i < tras[t]->nodes.size(); ++i) {
						Log(LogSwitch::FY) << tras[t]->nodes[i] << " ";
					}
					Log(LogSwitch::FY) << std::endl;
				}
			}
			int addTra(Traffic *tra) {
				tras.push_back(tra);
				int conflictsNum = 0;
				for (auto i = 0; i < tra->nodes.size() - 1; ++i) {
					ID node1 = tra->nodes[i], node2 = tra->nodes[i + 1];
					adjMat[node1][node2] = MaxDistance;
					if (adjMatTraNums[node1][node2] > 0) { conflictsNum++; }//说明原本有业务，超载量加1

					adjMatTraNums[node1][node2] ++;

				}
				conflictsOnAllLinks += conflictsNum;
				return conflictsNum;
			}

			int tryAddTra(List<ID> &nodes) {
				int conflictsNum = 0;
				for (auto i = 0; i < nodes.size() - 1; ++i) {
					ID node1 = nodes[i], node2 = nodes[i + 1];
					if (adjMatTraNums[node1][node2] > 0) { conflictsNum++; }//说明原本有业务，超载量加1
				}
				return conflictsNum;
			}
			int tryDelTra(Traffic *tra) {// 尝试删除tra，并获取减少的冲突数
				if (!tra) {
					Log(LogSwitch::FY) << "tryDelTra1 error" << std::endl;
					exit(-3);
				}
				int delta = 0;
				for (auto i = 0; i < tra->nodes.size() - 1; ++i) {
					ID node1 = tra->nodes[i], node2 = tra->nodes[i + 1];
					if (adjMatTraNums[node1][node2] - 1 > 0) {
						delta++;
					}
					if (adjMatTraNums[node1][node2] == 0) {
						Log(LogSwitch::FY) << "tryDelTra2 error" << std::endl;
						exit(-3);
					}
				}
				return delta;
			}
			int deleteTraWithoutChangeConflict(Traffic *tra) {
				if (!tra) {
					Log(LogSwitch::FY) << "traffic is NULL\n";
					exit(-1);
				}
				int res = 0;
				for (auto iter = tras.begin(); iter != tras.end(); ++iter) {
					if (*iter == tra) {
						tras.erase(iter);
						break;
					}
				}
				for (auto i = 0; i < tra->nodes.size() - 1; ++i) {
					ID node1 = tra->nodes[i], node2 = tra->nodes[i + 1];
					if (adjMatTraNums[node1][node2] == 0) {
						Log(LogSwitch::FY) << "adjMatTraNums error in deleteTra" << std::endl;
						exit(-2);
					}
					if (adjMatTraNums[node1][node2] > 1) {
						res++;
					}
					adjMatTraNums[node1][node2] --;

					if (adjMatTraNums[node1][node2] == 0) {
						adjMat[node1][node2] = 1;
					}
				}
				conflictsOnAllLinks -= res;
				return res;
			}

			int deleteTra(Traffic *tra,TwoWayList & conflictTras) { 
				int res = deleteTraWithoutChangeConflict(tra);
				groupConflicTras.delElem(tra->id);
				conflictTras.delElem(tra->id);
				return res;
			}
			int deleteTraNotChangeGlobal(Traffic *tra) { 
				int res = deleteTraWithoutChangeConflict(tra);
				groupConflicTras.delElem(tra->id);
				return res;
			}
		};
		
		struct Output {
		public:
			void save(List<Traffic> &traffics, int cost,String filename) {
				std::ofstream ofs;
				ofs.open(filename, std::ios::out);
				if (!ofs.is_open()) {
					std::cout << "[Fatal] Invalid answer path!" << std::endl;
				}
				else {
					ofs << cost << std::endl;
					ofs << "Lightpath -- Color -- RouteS " << std::endl;
					for (auto i = 0; i < traffics.size(); ++i) {
						ofs << i << " "<< traffics[i].wave <<" "<<traffics[i].nodes.size()<<" ";
						for (auto j = 0; j < traffics[i].nodes.size(); ++j) {
							ofs << traffics[i].nodes[j];
							if (j < traffics[i].nodes.size() - 1)
								ofs<< " ";
						}
						if (i < traffics.size() - 1)
							ofs << std::endl;
					}
				}
			}
		};

		struct Move {
		public:
			Traffic *tra;
			Wave old_w;
			Wave new_w;
		};
		struct ReroutingMove {
			Traffic *tra;
			List<ID> new_nodes;
			int delt;
		};
		struct EjectiveMove{
			Traffic *pickTra;
			List<ID> pickNewNodes;
			Traffic *reroutingTra;
			List<ID> reroutingNodes;
			int delt;
		};

#pragma endregion Type
#pragma	region RWAsolver::Model
		//class Model {
		//public:
		//	Model(List<Traffic *> &groupTras,int nodeNum,Topo &topo,GRBEnv *env,ModelType type,ModelObj mobj = MinOverLoad,int waveNum=0) {
		//		model = new GRBModel(*env);
		//		groupTrasNum = groupTras.size();
		//		this->nodeNum = nodeNum;

		//		addFlowVar(topo,type);
		//		addFlowConstraints(groupTras,topo);

		//		switch (mobj)
		//		{
		//		case szx::MinOverLoad: {
		//			setLinearObj(topo, type);
		//			traNumShouldRemove = model->get(GRB_DoubleAttr_ObjVal);
		//			break;
		//		}	
		//		case szx::MinConflictTra:
		//			break;
		//		case szx::MultiCommodity: {
		//			addCapacityConstraints(topo,waveNum);
		//			model->optimize();
		//			break;
		//		}
		//		default:
		//			break;
		//		}
		//	}
		//	~Model() {
		//		delete model;
		//	}
		//	void addCapacityConstraints(Topo &topo, int waveNum);
		//	void addFlowVar(Topo &topo,ModelType type);
		//	void addFlowConstraints(List<Traffic*> &groupTras, Topo &topo);
		//	void minTraNumShouldRemove(Topo &topo);
		//	void minFlow(Topo &topo);
		//	void setLinearObj(Topo &topo, ModelType type);
		//	void getAnswer(List<Traffic*> &groupTras, Topo &topo);

		//	int groupTrasNum;
		//	int nodeNum;
		//	int traNumShouldRemove;
		//	List<List<List<GRBVar>>> trafficOnLink;
		//	//for model1
		//	List<List<GRBVar>> auxVar;
		//	List<List<GRBVar>> shouldRemoveTra;
		//	//for model2
		//	List<GRBVar> deltas;
		//	GRBVar z;
		//	GRBModel *model;
		//};
#pragma endregion RWAsolver::Model

#pragma region Constant
	public:
		int nodeNum; // 网络中的结点个数
		int linkNum; // 网络中的链路数
		int trafficNum; // 总业务数
#pragma endregion Constant

#pragma region Constructor
	public:
		RWAsolver(const Problem::Input &inputData, const Environment &environment, const Configuration &config):input(inputData), env(environment), cfg(config), //rand(environment.randSeed),
			timer(std::chrono::milliseconds(environment.msTimeout)), iteration(1) //,tp(2)
		{
			evaluate_cnt = 0;
			/*grb_env = new GRBEnv();
			grb_env->set(GRB_IntParam_OutputFlag, 0);*/
			
		};

#pragma endregion Constructor

#pragma region Method
	public:
		~RWAsolver() {
			//delete grb_env;
		}
		bool solve();
		bool check(Length &obj) const;
		void record() const; // save running log.
		void switchNeighType(NeighBorType &neighType);
		bool optimize(Solution &sln, ID workerId = 0);
		void updateBestSolution();
		void perturb();
		void perturb_greedy();
		void init();
		void init_solution();
		void bfd_init_solution();
		void bfd_decide_init_solution();
		void bfd_decide_init_solution_withEjectionChain();
		void check_all(int flag =0);
		int check_groupConfAndAdjMatNum(WaveGroup & wavegroup, List<List<ID>> &adjMatTrasNum, int w);
		int getGroupConflictNum(WaveGroup & wavegroup, List<List<ID>> &adjMatTrasNum);
		void findConflicts(); // 在同一个波长盒子里判断是否会有冲突边
		int findEjectionMove(WaveGroup &wavegroup, Traffic * addedTra, EjectiveMove &ecmove);
		int EjectionChain(WaveGroup &wavegroup,Traffic *addedTra);
		int findBestRerouting(WaveGroup &wavegroup, TwoWayList &groupConflictTras, ReroutingMove &rrmove);
		int calculateIncomeForRerouting(List<List<ID>> &adjMatTraNums, Traffic *tra, List<ID> &new_nodes);
		void dijkstraMinConflicts(List<List<int>> &adjMatTraNums,Traffic *tra, int &conflictsNum, List<ID> &path);
		void RWAsolver::dijkstra(List<List<int>>& adjMat, Traffic * tra, List<ID> &path);
		void deployTrasByGraphColoring();
		void greedyDeployTras(); // 贪心放置业务
		void deployByFileAndSubColor();
		void deployByMemoryAndSubColor();
		void subColor(List<WaveGroup> &temp_waveGroups);
		void solveModelAndGetAnswer(WaveGroup &waveGroup);
		void get_mBest_bin(List<std::pair<ID, int>> &binValue, int m);
		Traffic *getOneConflictTra();
		int delTraFromOriginalBox(Traffic *conflictTra, TwoWayList &delBoxchangeTras); //将conflictTra从该业务所在颜色盒子中删除，并计算冲突下降值
		int tryAddTraToNewBoxByEjectionChain_Grasp(Traffic *tra, WaveGroup &wavegroup, TwoWayList& addTraNewTrasNodes, bool flag = false);
		int trydelTra_shaking(Traffic *delTra, TwoWayList &delBoxchangeTras);

		Traffic *pickConflictTraffic(WaveGroup &wavegroup, Traffic *addedTra = NULL);
		Traffic *pickMostConflictTraffic(WaveGroup &wavegroup, Traffic *addedTra =NULL);//从wavegroup的冲突业务中找出一条冲突数最多的业务(addedTra除外) 
		Traffic *pickRandomConflictTraffic(WaveGroup &wavegroup, Traffic *addedTra = NULL);// 从wavegroup的冲突业务中随机挑选一条(addedTra除外)
		void layeredFind_move();
		void exchangeTraffic();
		void exchangeTraffic_shaking();
		void selectKtraffics(List<Traffic *> &ptraffics, int k, List<Traffic *> &kTraffics, ID waveA=-1);
		void obtainMbins(Traffic *traA, int m, List<std::pair<ID, std::pair<int, int>>> &bin_value);
		void exchangeTraffic_all();
		void bestShiftMoveInMbins(Traffic *traA, int &bestw, int del_deltA, List<std::pair<ID, std::pair<int, int>>> &bin_value);
		Traffic* RWAsolver::bestExchangeMoveInMbins(Traffic *traA, int del_deltA, List<List<ID>> &bestGroupANodes, List<std::pair<ID, std::pair<int, int>>> &bin_value);
		void make_exchangeMove(Traffic *traA, List<ID> oldNodesA, Traffic *bestTraB, List<List<ID>> &bestGroupANodes);
		void merged_shift_exchange();
		void layeredFind_move_coarsed_grained();
		void layeredFind_move_all_by_dijkstra();
		void layeredFind_move_m_best();
		void layeredFind_move_m_best_original();
		void layeredFind_move_all_by_ejection();
		void make_shiftMove(int original_w, int best_w, TwoWayList &best_newTrasNodes, Traffic *conflictTra);

		void make_layerMove(int original_w,int best_w, TwoWayList &best_newTrasNodes,Traffic *conflictTra);
		void make_exchangeMove(int wA, int wB, TwoWayList &newTrasNodesA, TwoWayList &newTrasNodesB);
		void find_move(Move &move);
		void make_moveByModel(Move &move);
		void make_moveByDij(Move &move);
		void printWaveGroup(int w);
		void printWaveGroups();
#pragma endregion Method

#pragma region Field
	public:
		Environment env;
		Configuration cfg;
		Problem::Input input;
		Problem::Output output;
		double duration_time;
		//Random rand; // all random number in Solver must be generated by this.
		Timer timer; // the solve() should return before it is timeout.
		Iteration iteration;
		Iteration effectIterNum;
		struct GCAUX {
		public:
			List<ID> nodes;
			List<std::pair<ID, ID>> edges;
		}gc_aux; // 用于图着色的辅助数据结构

		int d_delt;
		Topo topo;
		List<Traffic> traffics;
		List<Traffic *> ptraffics;
		List<WaveGroup> waveGroups; // waveGroup[w] 代表使用了波长w的所有业务的集合
		List<WaveGroup> bestWaveGroups; // bestWaveGroups为历史最优解

		Output myOutput;
		//GRBEnv *grb_env;
		List<List<int>> color_tabu_table; // 颜色(波长)禁忌表
		int conflictsNum;
		int best_conflictsNum;
		int best_delt;
		int mCnt;
		int WaveNum;
		int noBeterTimes;
		int traNumShouldRemove;
		int sameSolutionCnt;
		List<std::pair<double, int>> process;
		List<std::pair<int, List<ID>>> greedy_deltNodes;
		int recordBestIter;

		int perturbInterval;
		int perturbGreedy_tprob;
		int perturbNum;
		int perturbRandomCnt;
		List<ID> groupChangedIter;
		List<List<ID>> recordIter; // recodIter[i][j]表示记录业务i到波长j冲突增量的时间戳
		List<List<ID>> delta; // delta[i][j]记录业务i到波长j冲突增量

		List<ID> best_groupChangedIter;
		List<List<ID>> best_recordIter;
		List<List<ID>> best_delta;
		List<List<ID>> leaveTime;
		List<List<ID>> best_leaveTime;
		TwoWayList delBoxchangeTras;
		TwoWayList best_delBoxchangeTras;
		TwoWayList best_newTrasNodes;
		TwoWayList tempNewNodes,tempNewNode2;
		TwoWayList currentNewTrasNodes;
		TwoWayList evaluatedTras;
		NeighBorType neighType;
		int dij_cnt;
		int evaluate_cnt;
		//ThreadPool  <> tp;
#pragma endregion Field
	}; // RWAsolver
	List<ID> graph_coloring(List<ID> nodes, List<std::pair<ID, ID>> edges,int colorNum);
}



