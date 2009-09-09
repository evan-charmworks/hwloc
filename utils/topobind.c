/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <topomask.h>
#include <hwloc.h>
#include <private/private.h>

#include <unistd.h>
#include <errno.h>
#include <assert.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: topobind [options] <location> -- command ...\n");
  fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
  fprintf(where, "            as supported by the topomask utility.\n");
  fprintf(where, "Options:\n");
  fprintf(where, "   --single\tbind on a single CPU to prevent migration\n");
  fprintf(where, "   --strict\trequire strict binding\n");
  fprintf(where, "   -v\t\tverbose messages\n");
  fprintf(where, "   --version\treport version and exit\n");
}

int main(int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_cpuset_t cpu_set; /* invalid until bind_cpus is set */
  int bind_cpus = 0;
  int single = 0;
  int verbose = 0;
  int flags = 0;
  int ret;
  char **orig_argv = argv;

  topo_cpuset_zero(&cpu_set);

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* skip argv[0], handle options */
  argv++;
  argc--;

  while (argc >= 1) {
    if (!strcmp(argv[0], "--")) {
      argc--;
      argv++;
      break;
    }

    if (*argv[0] == '-') {
      if (!strcmp(argv[0], "-v")) {
	verbose = 1;
	goto next;
      }
      else if (!strcmp(argv[0], "--help")) {
	usage(stdout);
	return EXIT_SUCCESS;
      }
      else if (!strcmp(argv[0], "--single")) {
	single = 1;
	goto next;
      }
      else if (!strcmp(argv[0], "--strict")) {
	flags |= TOPO_CPUBIND_STRICT;
	goto next;
      }
      else if (!strcmp (argv[0], "--version")) {
          printf("%s %s\n", orig_argv[0], VERSION);
          exit(EXIT_SUCCESS);
      }

      usage(stderr);
      return EXIT_FAILURE;
    }

    topomask_process_arg(topology, &topoinfo, argv[0], &cpu_set, verbose);
    bind_cpus = 1;

  next:
    argc--;
    argv++;
  }

  if (bind_cpus) {
    if (verbose)
      fprintf(stderr, "binding on cpu set %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(&cpu_set));
    if (single)
      topo_cpuset_singlify(&cpu_set);
    ret = topo_set_cpubind(topology, &cpu_set, flags);
    if (ret)
      fprintf(stderr, "topo_set_cpubind %"TOPO_PRIxCPUSET" failed (errno %d %s)\n", TOPO_CPUSET_PRINTF_VALUE(&cpu_set), errno, strerror(errno));
  }

  topo_topology_destroy(topology);

  if (!argc)
    return EXIT_SUCCESS;

  ret = execvp(argv[0], argv);
  if (ret && verbose)
    perror("execvp");
  return EXIT_FAILURE;
}
