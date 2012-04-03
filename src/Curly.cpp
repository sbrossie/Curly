//============================================================================
// Name        : HelloWorld.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "AbortIf.h"
#include "SelectCurler.h"

#include "Trace.h"
#include "LibEventCurler.h"
#include "ConnectionData.h"

static void
do_something_else() {
	usleep(10000);
}


static char**
get_connections_from_args(int argc, char * argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Needs to specify at least one connection");
		exit(1);
	}

	trace_info("STARTING with %d connections\n", argc -1);

	char ** connections = (char **) calloc(argc, sizeof(char *));
	ABORT_IF_NULL("calloc (connections)", connections);
	for (int i = 1; i < argc; i++) {
		connections[i-1] = argv[i];
		trace_debug("connections[%d] = %s\n", i - 1, connections[i-1]);
	}
	connections[argc - 1] = NULL;
	return connections;
}

static void
do_select_curl(char ** connections) {

	SelectCurler * curler = new SelectCurler();
	curler->addConnections(connections);

	int max_loop = 100;
	int done = 0;
	do {
		do_something_else();

		fd_set read_fd_set;
		FD_ZERO(&read_fd_set);

		fd_set write_fd_set;
		FD_ZERO(&write_fd_set);

		fd_set exc_fd_set;
		FD_ZERO(&exc_fd_set);
		done = curler->doSelect(&read_fd_set, &write_fd_set, &exc_fd_set);

	} while (max_loop -- > 0 && !done);
	delete(curler);
	trace_info("EXITING  done_with_all_connections = %d\n", done);
}

static void
do_lib_event_loop(char** input, int nb_entries) {

	LibEventCurler* curler  = new LibEventCurler();
	ConnectionData** connections = new ConnectionData*[nb_entries];
	ABORT_IF_NULL("new Connection[]", connections);
	int i = 0;
	char* cur_url = NULL;
	do  {
		cur_url = input[i];
		if (cur_url != NULL) {
			ConnectionData* cur_connection = new ConnectionData(cur_url);
			ABORT_IF_NULL("new Connection", cur_connection);
			connections[i] = cur_connection;
			i++;
		}
	} while (cur_url != NULL);
	connections[i] = NULL;
	curler->addConnections(connections);
	curler->loop();


	for (int i = 0; i < nb_entries; i++) {
		ConnectionData* curConnection = connections[i];
		if (curConnection != NULL) {
			trace_info("DONE with connection %s nbBytes = %d, httpError = %d\n",
					curConnection->getUrl(), curConnection->getNbBytes(), curConnection->getHttpStatus());
		}
		delete(connections[i]);
	}
	delete [] connections;
	delete(curler);
	trace_info("EXITING  done with loop\n");
}

int main(int argc, char * argv[]) {

	char ** connections = get_connections_from_args(argc, argv);

	//do_select_curl(connections);

	do_lib_event_loop(connections, argc);

	free(connections);
	return 0;
}

