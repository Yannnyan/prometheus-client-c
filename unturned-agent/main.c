/**
 * Copyright 2019 DigitalOcean Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "microhttpd.h"
#include "prom.h"
#include "promhttp.h"
#include "client.h"
#include <ctype.h>
#include "log.h"

prom_gauge_t * server_players_gauge;
prom_gauge_t * server_connection_state_gauge;
prom_gauge_t * server_status_gauge;
prom_gauge_t * game_current_buildid_gauge;
prom_gauge_t * game_steam_buildid_gauge;


//char * monitored_servers [] = {"LVMServer1","LVMServer2"};					     
//int monitored_servers_count = 2;
char * monitored_servers[20];
int monitored_servers_count;
const char* servers_path = "/management/game_servers/unturned/installation/Servers";

static void init(void) {
  // Initialize the Default registry
  prom_collector_registry_default_init();
  server_players_gauge = prom_collector_registry_must_register_metric(prom_gauge_new("unturned_server_players", "number of players in the server",1,(const char*[]){"serverName"}));
  server_connection_state_gauge = prom_collector_registry_must_register_metric(prom_gauge_new("unturned_server_connection", "server connection state",1,(const char*[]){"serverName"}));
  server_status_gauge = prom_collector_registry_must_register_metric(prom_gauge_new("unturned_server_state", "server state; eg running stopped",1,(const char*[]){"serverName"}));
  game_current_buildid_gauge = prom_collector_registry_must_register_metric(prom_gauge_new("unturned_game_current_buildid", "The current version of the game that is installed determined by buildid updated every 20 min",0,(const char*[]){NULL}));
  game_steam_buildid_gauge = prom_collector_registry_must_register_metric(prom_gauge_new("unturned_game_steam_buildid", "The current version published in steam determined by buildid updated every 20 min",0,(const char*[]){NULL}));
 
  // Set the active registry for the HTTP handler
  promhttp_set_active_collector_registry(NULL);
}


int get_server_port(char * server_name) {
   int len_name = strlen(server_name);
   char port[5];
   int i;
   for(i = 0; i < len_name; i++) {
     if (isdigit(server_name[i])) {
	break;
     }
   }
   return (strtol(server_name + i, NULL, 10) - 1) * 3 + 27017;
}

void updateServers() {
  char* cur_server;
  int server_port;
  char * response; int response_metric;
  log_info("Updating metrics...");
  for (int i =0; i < monitored_servers_count; i++) {
	cur_server = monitored_servers[i];
        server_port = get_server_port(cur_server); 
        // Get the status of the servers
	response = get_status(server_port,"127.0.0.1");
	response_metric = strcmp(response,"UP") == 0 ? 1 : 0;
	prom_gauge_set(server_status_gauge, response_metric, (const char *[]){cur_server});

	// Get the connection state of the servers
	response = get_connection(server_port, "127.0.0.1");
	response_metric = strcmp(response,"connected") == 0 ? 1 : 0;
	prom_gauge_set(server_connection_state_gauge, response_metric, (const char *[]){cur_server});

	// Get the user count of the servers
	response_metric = get_users_count(server_port, "127.0.0.1");
	prom_gauge_set(server_players_gauge, response_metric, (const char *[]){cur_server});
  }
 }

void updateGame() {
  FILE* fcbid,* fsbid;
  char * current_build_id_path = "/management/games_servers/unturned/scripts/current_buildid.txt";
  char * steam_build_id_path = "/management/games_servers/unturned/scripts/steamcmd_buildid.txt";
  fcbid = fopen(current_build_id_path, "r");
  fsbid = fopen(steam_build_id_path, "r");
  if (fcbid == NULL) {
	log_error("Could not open file to determine current build id %s", current_build_id_path);	
  }
  if (fsbid == NULL) {
	log_error("Could not open file to determine steam build id %s", steam_build_id_path);
  }
  char * endptr1, *endptr2;
  char * cbid=NULL, * sbid=NULL;
  ssize_t cbid_size=0, sbid_size=0;
  getline(&cbid, &cbid_size, fcbid);
  getline(&sbid, &sbid_size, fsbid);
  int cbuild = strtol(cbid, &endptr1, 10);
  int sbuild = strtol(sbid, &endptr2, 10);
  log_info("current %d steam %d", cbuild,sbuild);
  if (cbid == NULL || endptr1 == cbid) {
	log_error("Could not get line from the file current build id or parse the line to integer %s", current_build_id_path);
  }
  if (sbid == NULL || endptr2 == sbid) {
	log_error("Could not get line from the file steam build id or parse the line to integer %s", steam_build_id_path);
  }
  if (cbid != NULL) {
	prom_gauge_set(game_current_buildid_gauge, cbuild, (const char *[]){NULL});
	free(cbid);
  }
  if (sbid != NULL) {
	prom_gauge_set(game_steam_buildid_gauge, sbuild, (const char *[]){NULL});
	free(sbid);
  }
  fclose(fcbid);
  fclose(fsbid);
}
void set_env_variables() {
  log_info("Setting Environment Variables...");
  // Load envorinment variables
  char * env_monitored_servers = getenv("MONITORED_SERVERS");
  char * env_monitored_servers_count = getenv("MONITORED_SERVERS_COUNT");
  if (env_monitored_servers == NULL) { log_info("COULD NOT FIND ENVIRONMENT VARIABLE %s", "MONITORED_SERVERS"); exit(1);}
  if (env_monitored_servers_count == NULL) { log_info("COULD NOT FIND ENVIRONMENT VARIABLE %s", "MONITORED_SERVERS_COUNT"); exit(1);}
  
  // 
  char * endptr;
  monitored_servers_count = strtol(env_monitored_servers_count, &endptr, 10);
  if (endptr == env_monitored_servers_count) { log_info("COULD NOT SET ENVIRONMENT VARIABLE %s to INT", env_monitored_servers_count); exit(1);}

  // Load monitored servers from environment variable
  char * token = strtok(env_monitored_servers,",");
  int i =0, len=0;
  while (token != NULL) {
	len = strlen(token);
	monitored_servers[i] = calloc(len+ 1, 1);
	strcpy(monitored_servers[i], token);
	monitored_servers[i][len] = '\0';
	token = strtok(NULL,",");
	i++;
  }
}

int main(int argc, const char **argv) {
  init();
  log_info("Setting up PROM-HTTP Server...");
  struct MHD_Daemon *daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, 8000, NULL, NULL);
  if (daemon == NULL) {
    return 1;
  }
  set_env_variables();
  
  log_info("Setting Signal handlers for gracefull exit...");
  int done = 0;
  auto void intHandler(int signal);
  void intHandler(int signal) {
      printf("\nshutting down...\n");
      fflush(stdout);
      prom_collector_registry_destroy(PROM_COLLECTOR_REGISTRY_DEFAULT);
      MHD_stop_daemon(daemon);
      done = 1;
    }
  if (argc == 2) {
    unsigned int timeout = atoi(argv[1]);
    sleep(timeout);
    intHandler(0);
    return 0;
  }

  signal(SIGINT, intHandler);
  log_info("Starting to collect metrics...");
  while(done == 0) {
    updateGame();
    updateServers();
    sleep(60);
  }

  return 0;
}
