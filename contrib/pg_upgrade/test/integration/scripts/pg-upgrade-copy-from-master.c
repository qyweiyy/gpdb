#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "postgres_fe.h"
#include "utilities/pg-upgrade-copy.h"
#include "old_tablespace_file_parser.h"

static void
print_usage_header(char *message)
{
	printf("\n%s\n", message);
}

static void
print_usage_detail(char *message)
{
	printf("    %s\n", message);
}

static void
print_usage(void)
{
	print_usage_header("usage: ./pg-upgrade-copy-from-master --[prepare|enable]");
	print_usage_detail("--prepare : perform copy steps while preserving necessary configuration files");
	print_usage_detail("--enable  : place backed up configuration files in production location");
	print_usage_detail("");
	print_usage_header("not intended for production use. used in pre-production clusters only.");
	print_usage_detail("");
	print_usage_header("required options:");
	print_usage_detail("--master-host-username=[USERNAME] : username to be used by rsync to copy files from master");
	print_usage_detail("--master-hostname=[HOSTNAME] : hostname to be used by rsync to copy files from master");
	print_usage_detail("--master-data-directory=[PATH] : full path to the master's data directory");
	print_usage_detail("--old-master-gp-dbid=[INTEGER] : greenplum dbid used by the old master");
	print_usage_detail("--new-master-gp-dbid=[INTEGER] : greenplum dbid used by the new master");
	print_usage_detail("--new-segment-path=[PATH] : full path to the segment's data directory");
	print_usage_detail("--new-gp-dbid=[INTEGER] : greenplum dbid used by the new segment");
	print_usage_detail("--old-tablespace-mapping-file=[PATH] : path to the old tablespaces file generated by the upgrade of master");
}

typedef enum Command 
{
	Command_Prepare,
	Command_Enable,
	Command_NotSet
} Command;

static struct option long_options[] = {
	{"prepare", no_argument, NULL, 'p'},
	{"enable", no_argument, NULL, 'e'},
	{"master-host-username", required_argument, NULL, 'u'},
	{"master-hostname", required_argument, NULL, 'h'},
	{"master-data-directory", required_argument, NULL, 'd'},
	{"old-master-gp-dbid", required_argument, NULL, 'o'},
	{"new-master-gp-dbid", required_argument, NULL, 'n'},
	{"new-segment-path", required_argument, NULL, 's'},
	{"new-gp-dbid", required_argument, NULL, 'g'},
	{"old-tablespace-mapping-file-path", required_argument, NULL, 't'},
};

typedef struct Options {
		char *master_host_username;
		char *master_hostname;
		char *master_data_directory;
		int old_master_gp_dbid;
		int new_master_gp_dbid;
		char *new_segment_path;
		int new_gp_dbid;
		char *old_tablespace_mapping_file_path;
} Options;

void
OldTablespaceFileParser_invalid_access_error_for_field(int invalid_row_index, int invalid_field_index)
{
	printf("attempted to access invalid field: {row_index=%d, field_index=%d}",
	       invalid_row_index,
	       invalid_field_index);

	exit(1);
}

void
OldTablespaceFileParser_invalid_access_error_for_row(int invalid_row_index)
{
	printf("attempted to access invalid row: {row_index=%d}",
	       invalid_row_index);

	exit(1);
}

int
main(int argc, char *argv[])
{
	int option;
	int optindex = 0;

	char *string_option_not_set = "not set";
	int int_option_not_set = -1;

	Command command = Command_NotSet;
	Options options;
	options.master_host_username = string_option_not_set;
	options.master_hostname = string_option_not_set;
	options.master_data_directory = string_option_not_set;
	options.old_master_gp_dbid = int_option_not_set;
	options.new_master_gp_dbid = int_option_not_set;
	options.new_segment_path = string_option_not_set;
	options.old_tablespace_mapping_file_path = string_option_not_set;
	options.new_gp_dbid = int_option_not_set;

	while ((option = getopt_long(argc, argv, "p:e",
	                             long_options, &optindex)) != -1)
	{
		switch (option)
		{
			case 'p': 
				command = Command_Prepare;
				break;
			case 'e': 
				command = Command_Enable;
				break;
			case 'u':
				options.master_host_username = pg_strdup(optarg);
				break;
			case 'h':
				options.master_hostname = pg_strdup(optarg);
				break;
			case 'd': 
				options.master_data_directory = pg_strdup(optarg);
				break;
			case 'o': 
				options.old_master_gp_dbid = atoi(optarg);
				break;
			case 'n': 
				options.new_master_gp_dbid = atoi(optarg);
				break;
			case 's': 
				options.new_segment_path = pg_strdup(optarg);
				break;
			case 'g':
				options.new_gp_dbid = atoi(optarg);
				break;
			case 't':
				options.old_tablespace_mapping_file_path = pg_strdup(optarg);
				break;
			default:
				printf("unknown option given.");
				print_usage();
				exit(1);
		}
	};

	if (
		command == Command_NotSet ||
		strcmp(options.master_host_username, string_option_not_set) == 0 ||
		strcmp(options.master_hostname, string_option_not_set) == 0 ||
		strcmp(options.master_data_directory, string_option_not_set) == 0 ||
		options.old_master_gp_dbid == int_option_not_set ||
		options.new_master_gp_dbid == int_option_not_set ||
		strcmp(options.new_segment_path, string_option_not_set) == 0 ||
		options.new_gp_dbid == int_option_not_set ||
		strcmp(options.old_tablespace_mapping_file_path, string_option_not_set) == 0
	)
	{
		print_usage();
		exit(1);
	}

	PgUpgradeCopyOptions *copy_options = make_copy_options(
		options.master_host_username,
		options.master_hostname,
		options.master_data_directory,
		options.old_master_gp_dbid,
		options.new_master_gp_dbid,
		options.new_segment_path,
		options.new_gp_dbid,
		options.old_tablespace_mapping_file_path);

	if (command == Command_Enable)
		enable_segment_after_upgrade(copy_options);
	else if (command == Command_Prepare)
		prepare_segment_for_upgrade(copy_options);

	exit(0);
}