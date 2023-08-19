/*
 * lsgcpIS4Energy.c
 *
 *  Created on: 2013-6-4
 *      Author: jianghua
 */
/******************************************************************************/
/*                Graph coloring using techologies of Satz                    */
/*                Author: Chumin LI                                           */
/*                Copyright MIS, University of Picardie Jules Verne, France   */
/*                chu-min.li@u-picardie.fr                                    */
/*                November 2010                                               */
/******************************************************************************/

/* The program is distributed for research purpose, but without any guarantee.
 For any other (commercial or industrial) use of the proram, please contact
 Chumin LI
 */

/* Based on lsgcp1, introduce random walk
 */

/* Based on lsgcd2, introduce promising decreasing moves
 */

/* identitic as lsgcp3, check consistency
 */

/* Based on lsgcp6, when a node quit a color c, it cannot be re-colored
 by c, unless at least one of its neibors changes color
 */

/* Based on lsgcp9, introduce a new type move,ZERO-MOVE:
 *  A node V ,current color is C1, if there exist a color C2(C1!=C2)
 *  makes node_score[V][C2]==0 and CLASS_SIZE(C2)>=CLASS_SIZE(C1),where CLASS_SIZE(Ci) is the count of
 *  nodes which has color Ci.
 */
/*
 * Based on lsgcp9CNoise,add max indepent set extracting.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
 //#include <sys/times.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include"RWAsolver.h"


#define WORD_LENGTH 100
#define TRUE 1
#define FALSE 0
#define NONE -1

/* the tables of nodes and edges are statically allocated. Modify the
 parameters tab_node_size and tab_edge_size before compilation if
 necessary */
#define tab_node_size  3000
#define tab_edge_size 4500000
#define max_nb_value 500
#define pop(stack) stack[--stack ## _fill_pointer]
#define push(item, stack) stack[stack ## _fill_pointer++] = item
#define random_integer(max) (((unsigned long int)rand())%(max))
#define PASSIVE 0
#define ACTIVE 1
#define is_zero_move(node,color) (node_score[node][color] == 0 && color != node_value[node])

#define push_move_stack(node,color) if (node_score[node][color] == 1) {\
				                                             push(node, PRM_MOVE_STACK);\
				                                             push(color, PRM_MOVE_STACK);\
			                                                 } else if (is_zero_move(node,color)) {\
				                                             push(node, ZRO_MOVE_STACK);\
				                                             push(color, ZRO_MOVE_STACK);\
			                                                 }

#define push_zero_move(node,color)  if (is_zero_move(node,color)) {\
				                                             push(node, ZRO_MOVE_STACK);\
				                                             push(color, ZRO_MOVE_STACK);\
			                                                 }

#define push_prm_move(node,color)  if (node_score[node][color] == 1) {\
				                                             push(node, PRM_MOVE_STACK);\
				                                             push(color, PRM_MOVE_STACK);\
			                                                 }

namespace szx {
	int *node_neibors[tab_node_size];
	int node_value[tab_node_size];
	int node_score[tab_node_size][max_nb_value];
	int node_conflict[tab_node_size];
	long long flip_time[tab_node_size][max_nb_value];
	long long leave_time[tab_node_size][max_nb_value];
	//int node_time[tab_node_size];
	int tabu[tab_node_size];
	int PRM_MOVE_STACK[2 * tab_node_size * max_nb_value];
	int PRM_MOVE_STACK_fill_pointer = 0;

	int nb_neibors[tab_node_size];

	int EDGE[tab_edge_size][2];
	int EDGE_STACK[tab_edge_size];
	int EDGE_STACK_fill_pointer = 0;

	int index_in_EDGE_STACK[tab_edge_size];

	int NB_VALUE, NB_NODE, NB_EDGE;

	long long MAXSTEPS = 100;
	int MAXTRIES = 1, NOISE = 50, SEED = 0, SEED_FLAG = FALSE, LNOISE = 5, ZNOISE =
		40, SNOISE = 75;
	int FORMAT = 1;
	int HELP_FLAG = FALSE;
	char *INPUT_FILE;
	long long nbwalk = 0;
	long long nbprm = 0;
	long long nbzero = 0;
	long long var1, var2;

	// the size of color class
	int class_size[max_nb_value];
	int ZRO_MOVE_STACK[2 * tab_node_size * max_nb_value];
	int ZRO_MOVE_STACK_fill_pointer = 0;

	void modify_seed() {
		/*int seed;
		struct timeval tv;
		struct timezone tzp;
		if (SEED_FLAG == TRUE) {
			srand(SEED);
			SEED = SEED + 17;
			if (SEED == 0)
				SEED = 17;
		} else {
			gettimeofday(&tv, &tzp);
			seed = ((tv.tv_sec & 0177) * 1000000) + tv.tv_usec;
			srand(seed);
		}*/
	}

	void process_info(int argc, char *argv[]) {
		int i = 0;
		time_t rawtime;
		time(&rawtime);
		printf(
			"-------------------------------------------------------------------------------------------------\n");
		printf("starting time: %s", asctime(localtime(&rawtime)));
		printf("command  line: ");
		for (i = 0; i < argc; i++) {
			printf("%s ", argv[i]);
		}
		printf("\n");
		printf(
			"-------------------------------------------------------------------------------------------------\n");
	}

	int edge_redundant(int node1, int node2, int nb_edges) { /* node1<node2 */
		int i;
		for (i = 0; i < nb_edges; i++) {
			if (node1 == EDGE[i][0] && node2 == EDGE[i][1])
				return TRUE;
		}
		return FALSE;
	}

	int build_simple_graph_instance(List<ID> nodes, List<std::pair<ID,ID>> edges) {
		
		char ch, word2[WORD_LENGTH];
		int i, j, e, node1, node2, *neibors, redundant;
		
		NB_NODE = nodes.size();
		NB_EDGE = edges.size();
		

		for (i = 0; i < NB_EDGE; i++) {
			EDGE[i][0] = edges[i].first;
			EDGE[i][1] = edges[i].second;
			if (EDGE[i][0] == EDGE[i][1]) {
				i--;
				NB_EDGE--;
			}
			else {
				if (EDGE[i][0] > EDGE[i][1]) {
					e = EDGE[i][1];
					EDGE[i][1] = EDGE[i][0];
					EDGE[i][0] = e;
				}
				nb_neibors[EDGE[i][0]]++;
				nb_neibors[EDGE[i][1]]++;
			}
		}

		for (i = 0; i < NB_NODE; i++) {
			node_neibors[i] = (int *)malloc((2 * nb_neibors[i] + 1) * sizeof(int));
			node_neibors[i][2 * nb_neibors[i]] = NONE;
			nb_neibors[i] = 0;
		}
		for (i = 0; i < NB_EDGE; i++) {
			node1 = EDGE[i][0];
			node2 = EDGE[i][1];
			neibors = node_neibors[node1];
			redundant = FALSE;
			for (j = 0; j < 2 * nb_neibors[node1]; j += 2)
				if (neibors[j] == node2) {
					printf("edge redundant %d \n", i);
					redundant = TRUE;
					break;
				}
			if (redundant == FALSE) {
				node_neibors[node1][2 * nb_neibors[node1]] = node2;
				node_neibors[node1][2 * nb_neibors[node1] + 1] = i;
				nb_neibors[node1]++;
				node_neibors[node2][2 * nb_neibors[node2]] = node1;
				node_neibors[node2][2 * nb_neibors[node2] + 1] = i;
				nb_neibors[node2]++;
			}
		}
		for (i = 0; i < NB_NODE; i++) {
			node_neibors[i][2 * nb_neibors[i]] = NONE;
		}
		return TRUE;
	}
	int build_simple_graph_instanceByFile(char *input_file) {
		FILE* fp_in = fopen(input_file, "r");
		char ch, word2[WORD_LENGTH];
		int i, j, e, node1, node2, *neibors, redundant;
		if (fp_in == NULL)
			return FALSE;
		if (FORMAT == 1) {
			fscanf(fp_in, "%c", &ch);
			while (ch != 'p') {
				while (ch != '\n')
					fscanf(fp_in, "%c", &ch);
				fscanf(fp_in, "%c", &ch);
			}
			fscanf(fp_in, "%s%d%d", word2, &NB_NODE, &NB_EDGE);
		}
		else
			fscanf(fp_in, "%d%d", &NB_NODE, &NB_EDGE);

		for (i = 0; i < NB_EDGE; i++) {
			if (FORMAT == 1)
				fscanf(fp_in, "%s%d%d", word2, &EDGE[i][0], &EDGE[i][1]);
			else
				fscanf(fp_in, "%d%d", &EDGE[i][0], &EDGE[i][1]);
			if (EDGE[i][0] == EDGE[i][1]) {
				//printf("auto edge %d over %d\n", i--, NB_EDGE--);
				i--;
				NB_EDGE--;
			}
			else {
				if (EDGE[i][0] > EDGE[i][1]) {
					e = EDGE[i][1];
					EDGE[i][1] = EDGE[i][0];
					EDGE[i][0] = e;
				}
				nb_neibors[EDGE[i][0]]++;
				nb_neibors[EDGE[i][1]]++;
			}
		}
		fclose(fp_in);

		for (i = 0; i < NB_NODE; i++) {
			node_neibors[i] = (int *)malloc((2 * nb_neibors[i] + 1) * sizeof(int));
			node_neibors[i][2 * nb_neibors[i]] = NONE;
			nb_neibors[i] = 0;
		}
		for (i = 0; i < NB_EDGE; i++) {
			node1 = EDGE[i][0];
			node2 = EDGE[i][1];
			neibors = node_neibors[node1];
			redundant = FALSE;
			for (j = 0; j < 2 * nb_neibors[node1]; j += 2)
				if (neibors[j] == node2) {
					printf("edge redundant %d \n", i);
					redundant = TRUE;
					break;
				}
			if (redundant == FALSE) {
				node_neibors[node1][2 * nb_neibors[node1]] = node2;
				node_neibors[node1][2 * nb_neibors[node1] + 1] = i;
				nb_neibors[node1]++;
				node_neibors[node2][2 * nb_neibors[node2]] = node1;
				node_neibors[node2][2 * nb_neibors[node2] + 1] = i;
				nb_neibors[node2]++;
			}
		}
		for (i = 0; i < NB_NODE; i++) {
			node_neibors[i][2 * nb_neibors[i]] = NONE;
		}
		return TRUE;
	}
	int verify_solution() {
		int i;
		for (i = 0; i < NB_EDGE; i++) {
			if (node_value[EDGE[i][0]] == node_value[EDGE[i][1]])
				return FALSE;
		}
		return TRUE;
	}

	void update_prm_moves() {
		int i, j;
		for (i = 0; i < PRM_MOVE_STACK_fill_pointer; i += 2)
			if (node_score[PRM_MOVE_STACK[i]][PRM_MOVE_STACK[i + 1]] <= 0)
				break;
		for (j = i + 2; j < PRM_MOVE_STACK_fill_pointer; j += 2) {
			if (node_score[PRM_MOVE_STACK[j]][PRM_MOVE_STACK[j + 1]] > 0) {
				PRM_MOVE_STACK[i] = PRM_MOVE_STACK[j];
				PRM_MOVE_STACK[i + 1] = PRM_MOVE_STACK[j + 1];
				i += 2;
			}
		}
		PRM_MOVE_STACK_fill_pointer = i;
	}

	void update_zero_moves() {
		int i, j;
		for (i = 0; i < ZRO_MOVE_STACK_fill_pointer; i += 2)
			if (!is_zero_move(ZRO_MOVE_STACK[i], ZRO_MOVE_STACK[i + 1]))
				break;
		for (j = i + 2; j < ZRO_MOVE_STACK_fill_pointer; j += 2) {
			if (is_zero_move(ZRO_MOVE_STACK[j], ZRO_MOVE_STACK[j + 1])) {
				ZRO_MOVE_STACK[i] = ZRO_MOVE_STACK[j];
				ZRO_MOVE_STACK[i + 1] = ZRO_MOVE_STACK[j + 1];
				i += 2;
			}
		}
		ZRO_MOVE_STACK_fill_pointer = i;
	}

	void flip_and_update_score(int node, int new_color, int old_color,
		long long flip) {
		int neibor, *neibors, c, e, last_conflicting_edge, index, new_score,
			*scores;
		node_value[node] = new_color;
		flip_time[node][new_color] = flip;
		leave_time[node][old_color] = flip;
		//node_time[node] = flip;
		class_size[old_color]--;
		class_size[new_color]++;
		tabu[node] = old_color;
		scores = node_score[node];
		new_score = scores[new_color];
		for (c = 0; c < NB_VALUE; c++)
			scores[c] = scores[c] - new_score;
		if (new_score != 0) {
			node_conflict[node] -= new_score;
			for (c = 0; c < NB_VALUE; c++)
				push_zero_move(node, c);
		}
		neibors = node_neibors[node];
		for (neibor = *neibors; neibor != NONE; neibor = *(neibors += 2)) {
			tabu[neibor] = NONE;
			scores = node_score[neibor];
			if (node_value[neibor] == old_color) {
				node_conflict[neibor]--;
				last_conflicting_edge = pop(EDGE_STACK);
				e = *(neibors + 1);
				index = index_in_EDGE_STACK[e];
				EDGE_STACK[index] = last_conflicting_edge;
				index_in_EDGE_STACK[last_conflicting_edge] = index;
				for (c = 0; c < NB_VALUE; c++)
					if (c != old_color) {
						scores[c]--;
						push_zero_move(neibor, c);
					}
				scores[new_color]--;
				push_zero_move(neibor, new_color);
			}
			else if (node_value[neibor] == new_color) {
				node_conflict[neibor]++;
				e = *(neibors + 1);
				index_in_EDGE_STACK[e] = EDGE_STACK_fill_pointer;
				push(e, EDGE_STACK);
				for (c = 0; c < NB_VALUE; c++)
					if (c != new_color) {
						scores[c]++;
						push_move_stack(neibor, c);
					}
				scores[old_color]++;
				push_move_stack(neibor, old_color);
			}
			else {
				scores[old_color]++;
				push_move_stack(neibor, old_color);
				scores[new_color]--;
				push_zero_move(neibor, new_color);
			}
		}
		update_prm_moves();
		update_zero_moves();
	}

	void random_walk(int conflicting_edge, int *nodeColor) {
		int choice, node, c, current_color;

		choice = random_integer(2 * NB_VALUE - 2);
		if (choice < NB_VALUE - 1)
			node = EDGE[conflicting_edge][0];
		else {
			node = EDGE[conflicting_edge][1];
			choice = choice - NB_VALUE + 1;
		}
		current_color = node_value[node];
		for (c = 0; c < NB_VALUE; c++) {
			if (c != current_color) {
				if (choice == 0) {
					nodeColor[0] = node;
					nodeColor[1] = c;
					return;
				}
				else
					choice--;
			}
		}
	}
	void choose_node_color(int *nodeColor) {
		int node, i, conflicting_edge, nb, c, energy, max_nb = -NB_EDGE,
			second_max = NONE, second_best_node = NONE, second_color = NONE,
			best_node = NONE, best_color = NONE, chosen_node, chosen_color,
			conflicting_edge_index, current_color, best_node_energy = NB_NODE,
			second_node_energy = NB_NODE;
		long long recent_time = 0, ftime = 0, best_flip_time = MAXSTEPS,
			second_flip_time = MAXSTEPS;
		conflicting_edge_index = random_integer(EDGE_STACK_fill_pointer);
		conflicting_edge = EDGE_STACK[conflicting_edge_index];
		if (random_integer(100) < LNOISE) {
			random_walk(conflicting_edge, nodeColor);
			return;
		}
		for (i = 0; i < 2; i++) {
			node = EDGE[conflicting_edge][i];
			current_color = node_value[node];
			energy = node_conflict[node];
			for (c = 0; c < NB_VALUE; c++) {
				if (c != current_color && tabu[node] != c) {
					nb = node_score[node][c];
					ftime = leave_time[node][c];
					if (nb > max_nb
						|| (nb == max_nb
							&& (energy > best_node_energy
								|| (energy == best_node_energy
									&& ftime < best_flip_time)))) {
						second_best_node = best_node;
						second_color = best_color;
						second_max = max_nb;
						second_flip_time = best_flip_time;
						second_node_energy = best_node_energy;
						best_node = node;
						best_color = c;
						max_nb = nb;
						best_flip_time = ftime;
						best_node_energy = energy;
					}
					else if (nb > second_max
						|| (nb == second_max
							&& (energy > second_node_energy
								|| (energy == second_node_energy
									&& ftime < second_flip_time)))) {
						second_max = nb;
						second_flip_time = ftime;
						second_node_energy = energy;
						second_best_node = node;
						second_color = c;
					}
					if (ftime > recent_time)
						recent_time = ftime;
				}
			}
		}
		if (recent_time == best_flip_time) {
			if (random_integer(100) < SNOISE) {
				chosen_node = second_best_node;
				chosen_color = second_color;
				var1++;
			}
			else {
				chosen_node = best_node;
				chosen_color = best_color;
			}
		}
		else {
			chosen_node = best_node;
			chosen_color = best_color;
		}
		nodeColor[0] = chosen_node;
		nodeColor[1] = chosen_color;

		var2++;
	}

	void initialize_colors(int nb_colors) {
		int node, c, i, *neibors, neibor, nb_conflicts;
		modify_seed();
		for (c = 0; c < nb_colors; c++) {
			class_size[c] = 0;
		}
		for (node = 0; node < NB_NODE; node++) {
			node_value[node] = random_integer(nb_colors);
			class_size[node_value[node]]++;
			node_conflict[node] = 0;
			tabu[node] = NONE;
			//node_time[node] = NONE;
		}
		EDGE_STACK_fill_pointer = 0;
		PRM_MOVE_STACK_fill_pointer = 0;
		ZRO_MOVE_STACK_fill_pointer = 0;
		for (i = 0; i < NB_EDGE; i++)
			if (node_value[EDGE[i][0]] == node_value[EDGE[i][1]]) {
				index_in_EDGE_STACK[i] = EDGE_STACK_fill_pointer;
				push(i, EDGE_STACK);
			}
		for (node = 0; node < NB_NODE; node++)
			for (c = 0; c < nb_colors; c++) {
				node_score[node][c] = 0;
				flip_time[node][c] = 0;
				leave_time[node][c] = 0;
			}
		for (node = 0; node < NB_NODE; node++) {
			neibors = node_neibors[node];
			for (neibor = *neibors; neibor != NONE; neibor = *(neibors += 2))
				node_score[node][node_value[neibor]]++;
			nb_conflicts = node_score[node][node_value[node]];
			node_conflict[node] = nb_conflicts;
			for (c = 0; c < nb_colors; c++) {
				node_score[node][c] = nb_conflicts - node_score[node][c];
				if (node_score[node][c] > 0) {
					push(node, PRM_MOVE_STACK);
					push(c, PRM_MOVE_STACK);
				}
				else if (is_zero_move(node, c)) {
					push(node, ZRO_MOVE_STACK);
					push(c, ZRO_MOVE_STACK);
				}
			}
		}
	}

	int invPhi = 10;
#define invTheta 5

	long long lastAdaptFlip, lastBest, AdaptLength, NB_ADAPT, NB_BETTER;
	int lastEvenSteps, lastUnEvenSteps;
	int nb_even, nb_uneven, uneven_var_threshold = 9, nb_uneven_var;

	void initNoise() {
		lastAdaptFlip = 0;
		lastEvenSteps = 0;
		lastUnEvenSteps = 0;
		lastBest = EDGE_STACK_fill_pointer;
		NOISE = 0;
		LNOISE = 0;
		NB_BETTER = 0;
		NB_ADAPT = 0;
		AdaptLength = NB_EDGE / invTheta;
	}

	void adaptNoise(long long flip) {
		if (EDGE_STACK_fill_pointer < lastBest) {
			NOISE -= (int)(NOISE / (2 * invPhi));
			LNOISE = (int)NOISE / 10;
			//ZNOISE -= (int) (ZNOISE / (2 * invPhi));
			lastAdaptFlip = flip;
			lastBest = EDGE_STACK_fill_pointer;
		}
		else if ((flip - lastAdaptFlip) > AdaptLength) {
			NOISE += (int)((100 - NOISE) / invPhi);
			//ZNOISE += (int) ((100 - ZNOISE) / invPhi);
			LNOISE = (int)NOISE / 10;
			lastAdaptFlip = flip;
			lastBest = EDGE_STACK_fill_pointer;
		}
	}
	//int choose_best_zero_move(int *nodeColor) {
	//	int i, node, color, index = -1, score = 0, best_score = GRADIENT, max_size =
	//			0, best_time = MAXSTEPS;
	//	for (i = 0; i < ZRO_MOVE_STACK_fill_pointer; i += 2) {
	//		node = ZRO_MOVE_STACK[i];
	//		color = ZRO_MOVE_STACK[i + 1];
	//		score = class_size[color] - class_size[node_value[node]];
	//		if (score > best_score) {
	//			index = i;
	//			best_score = score;
	//			best_time = flip_time[node][color];
	//			max_size = class_size[color];
	//		} else if (score == best_score && class_size[color] > max_size) {
	//			index = i;
	//			best_score = score;
	//			best_time = flip_time[node][color];
	//			max_size = class_size[color];
	//		} else if (score == best_score && class_size[color] == max_size
	//				&& flip_time[node][color] < best_time) {
	//			index = i;
	//			best_score = score;
	//			best_time = flip_time[node][color];
	//			max_size = class_size[color];
	//		}
	//	}
	//	if (best_score > GRADIENT) {
	//		nodeColor[0] = ZRO_MOVE_STACK[index];
	//		nodeColor[1] = ZRO_MOVE_STACK[index + 1];
	//		return TRUE;
	//	} else {
	//		return FALSE;
	//	}
	//}

	int choose_best_zero_move(int *nodeColor) {
		int i, node, color, score = 0;
		int best_conf_index = -1, best_conf_score = 0, best_conf_size = 0;
		int best_zero_index = -1, best_zero_score = 0, best_zero_size = 0;
		long long best_zero_time = MAXSTEPS, best_conf_time = MAXSTEPS;
		for (i = 0; i < ZRO_MOVE_STACK_fill_pointer; i += 2) {
			node = ZRO_MOVE_STACK[i];
			color = ZRO_MOVE_STACK[i + 1];
			score = class_size[color] - class_size[node_value[node]];
			if (score >= 0) {
				if (node_conflict[node] == 0) {
					score = class_size[color];
					if (score > best_zero_score) {
						best_zero_index = i;
						best_zero_score = score;
						best_zero_size = class_size[node_value[node]];
						best_zero_time = flip_time[node][color];
					}
					else if (score == best_zero_score
						&& class_size[node_value[node]] < best_zero_size) {
						best_zero_index = i;
						best_zero_size = class_size[node_value[node]];
						best_zero_time = flip_time[node][color];
					}
					else if (score == best_zero_score
						&& class_size[node_value[node]] == best_zero_size
						&& flip_time[node][color] < best_zero_time) {
						best_zero_index = i;
						best_zero_time = flip_time[node][color];
					}
				}
				else if (node_conflict[node] == 1) {
					if (score > best_conf_score) {
						best_conf_index = i;
						best_conf_score = score;
						best_conf_size = class_size[color];
						best_conf_time = flip_time[node][color];
					}
					else if (score == best_conf_score
						&& class_size[color] > best_conf_size) {
						best_conf_index = i;
						best_conf_size = class_size[color];
						best_conf_time = flip_time[node][color];
					}
					else if (score == best_conf_score
						&& class_size[color] == best_conf_size
						&& flip_time[node][color] < best_conf_time) {
						best_conf_index = i;
						best_conf_time = flip_time[node][color];
					}
				}
			}
		}
		if (best_zero_index > -1) {
			nodeColor[0] = ZRO_MOVE_STACK[best_zero_index];
			nodeColor[1] = ZRO_MOVE_STACK[best_zero_index + 1];
			return TRUE;
		}
		else if (best_conf_index > -1 && random_integer(100) < ZNOISE) {
			nodeColor[0] = ZRO_MOVE_STACK[best_conf_index];
			nodeColor[1] = ZRO_MOVE_STACK[best_conf_index + 1];
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	int choose_best_prm_move(int *nodeColor) {
		int i, index = -1;
		long long ftime = MAXSTEPS, time = 0;
		if (PRM_MOVE_STACK_fill_pointer == 2) {
			nodeColor[0] = PRM_MOVE_STACK[0];
			nodeColor[1] = PRM_MOVE_STACK[1];
			return TRUE;
		}
		for (i = 0; i < PRM_MOVE_STACK_fill_pointer; i += 2) {
			//time = flip_time[PRM_MOVE_STACK[i]][PRM_MOVE_STACK[i + 1]];
			time = leave_time[PRM_MOVE_STACK[i]][PRM_MOVE_STACK[i + 1]];
			if (time < ftime) {
				index = i;
				ftime = time;
			}
		}
		nodeColor[0] = PRM_MOVE_STACK[index];
		nodeColor[1] = PRM_MOVE_STACK[index + 1];
		return TRUE;
	}
	char * get_filename(char * pathname) {
		int i = strlen(pathname), len = i;
		while (pathname[--i] != '/') {
			if (i == 0)
				break;
		}
		if (pathname[i] == '/')
			return (pathname + i + 1);
		else
			return pathname;
	}
	List<ID> print_solution() {
		List<ID> res;
		res.reserve(NB_NODE);
		for (auto i = 0; i < NB_NODE; i++) {
			res.push_back(node_value[i]);
			//fprintf(fp_sol, "%d ", node_value[i] + 1);
		}
		return res;
	}

	List<ID> search_by_ls() {
		List<ID> res;
		int i;
		long long j;
		int min[1024], depth, nodeColor[2], verify, nb_suc = 0, init_seed;
		long long total_flip = 0;
		//struct rusage starttime0, starttime, endtime;
		//getrusage(RUSAGE_SELF, &starttime0);
		init_seed = SEED;


		for (i = 0; i < MAXTRIES; i++) {
			min[i] = NB_EDGE, depth = 0, nbwalk = 0, nbprm = 0, nbzero = 0;
			initNoise();
			initialize_colors(NB_VALUE);
			var1 = 0, var2 = 0;
			for (j = 1; j < MAXSTEPS; j++)	 {
				if (EDGE_STACK_fill_pointer == 0)
					break;
				if (PRM_MOVE_STACK_fill_pointer > 0) {
					choose_best_prm_move(nodeColor);
					nbprm++;
				}
				else if (ZRO_MOVE_STACK_fill_pointer
		> 0 && choose_best_zero_move(nodeColor) == TRUE) {
					nbzero++;
				}
				else {
					choose_node_color(nodeColor);
					nbwalk++;
				}

				flip_and_update_score(nodeColor[0], nodeColor[1],
					node_value[nodeColor[0]], j);
				adaptNoise(j);
				if (EDGE_STACK_fill_pointer < min[i])
					min[i] = EDGE_STACK_fill_pointer;
			}
			verify = FALSE;
			if (EDGE_STACK_fill_pointer == 0 && verify_solution() == TRUE) {
				Log(LogSwitch::Color) << "satisfy\n";
				verify = TRUE;
			}
			nb_suc++;
			total_flip += j;
			res = print_solution();
			//}

		}

		return res;
	}

	void scanone(int argc, char *argv[], int i, int *varptr) {
		if (i >= argc || sscanf(argv[i], "%i", varptr) != 1) {
			fprintf(stderr, "Bad argument %s\n",
				i < argc ? argv[i] : argv[argc - 1]);
			exit(-1);
		}
	}
	void scanoneLL(int argc, char *argv[], int i, long long *varptr) {
		if (i >= argc || sscanf(argv[i], "%lld", varptr) != 1) {
			fprintf(stderr, "Bad argument %s\n",
				i < argc ? argv[i] : argv[argc - 1]);
			exit(-1);
		}
	}
	void scanoneU(int argc, char *argv[], int i, unsigned int *varptr) {
		if (i >= argc || sscanf(argv[i], "%u", varptr) != 1) {
			fprintf(stderr, "Bad argument %s\n",
				i < argc ? argv[i] : argv[argc - 1]);
			exit(-1);
		}
	}

	void parse_parameters(int argc, char *argv[]) {
		int i;
		if (argc < 2)
			HELP_FLAG = TRUE;
		else
			for (i = 1; i < argc; i++) {
				if (strcmp(argv[i], "-n") == 0)
					scanone(argc, argv, ++i, &NB_VALUE);
				else if (strcmp(argv[i], "-f") == 0)
					scanone(argc, argv, ++i, &FORMAT);
				else if (strcmp(argv[i], "-z") == 0)
					scanone(argc, argv, ++i, &ZNOISE);
				else if (strcmp(argv[i], "-s") == 0)
					scanone(argc, argv, ++i, &SNOISE);
				else if (strcmp(argv[i], "-h") == 0)
					HELP_FLAG = TRUE;
				else if (strcmp(argv[i], "-t") == 0)
					scanone(argc, argv, ++i, &MAXTRIES);
				else if (strcmp(argv[i], "-c") == 0)
					scanoneLL(argc, argv, ++i, &MAXSTEPS);
				else if (strcmp(argv[i], "-seed") == 0) {
					scanone(argc, argv, ++i, &SEED);
					SEED_FLAG = TRUE;
				}
				else
					INPUT_FILE = argv[i];
			}
	}

	/*int main(int argc, char *argv[]) {
		process_info(argc, argv);
		parse_parameters(argc, argv);
	}
*/

	List<ID> graph_coloring(List<ID> nodes, List<std::pair<ID, ID>> edges, int colorNum)
	{
		List<ID> res;
		MAXSTEPS = 250000;
		printf("%lld\n", MAXSTEPS);
		NB_VALUE = colorNum;
		build_simple_graph_instance(nodes,edges);
		//build_simple_graph_instanceByFile("F://组合优化//rwa//graphColoring//Deploy//Instances//DSJC500.5.col");
		res = search_by_ls();
		return res;
	}

}

