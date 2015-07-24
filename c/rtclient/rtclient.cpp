/*

* Author:       Rick Candell (rick.candell@nist.gov)
* 
* Organization: National Institute of Standards and Technology
*               U.S. Department of Commerce
* License:      Public Domain

*/
// rtclient.cpp : Defines the main() entry point for the console application.
//

#include <boost/system/config.hpp>
#include <boost/program_options.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#define XMV_SHMEM_NAME ("xmv_shmem")
#define IDV_SHMEM_NAME ("idv_shmem")
#define SIM_SHMEM_NAME ("sim_shmem")

#include <fstream>
#include <iostream>
#include <iomanip>
#include <math.h> 
#include <utility>

#include "TEPlant.h"

typedef struct { int index; double value; } xmv_pair;
typedef struct { int index; int value; } idv_pair;

int main(int argc, char* argv[])
{
	// program option variables
	bool set_idv = false, unset_idv = false, set_xmv = false;
	unsigned idv_index = 0, xmv_index = 0;
	double xmv_value = 0.0;
	bool print_all = false;

	// program options
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h",													"print the help message")
		("setidv,i",		po::value<unsigned>(&idv_index),		"id (1-based) of the disturbance to enable")
		("unsetidv,u",		po::value<unsigned>(&idv_index),		"id (1-based) of the disturbance to disable")
		("xmv-index,x",		po::value<unsigned>(&xmv_index),		"index (1-based) of the new xmv/setpoint")
		("xmv-value,v",		po::value<double>(&xmv_value),			"value for the new xmv/setpoint")
		("print-all-vars,p", po::bool_switch(&print_all)->default_value(false), "list the measured values")
		;

	po::variables_map vm;
	try	{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		if (vm.count("help"))
		{
			std::cout << desc << std::endl;
			return 0;
		}

		po::notify(vm);
	}
	catch (po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return 0;
	}

	using namespace boost::interprocess;
	try
	{
		if (set_xmv)
		{
			shared_memory_object xmv_shm(open_only, XMV_SHMEM_NAME, read_write);
			mapped_region reg_xmv(xmv_shm, read_write);

			xmv_pair xmv_update;
			xmv_update.index = xmv_index - 1;
			xmv_update.value = xmv_value;
			std::cout << "setting xmv " << xmv_index << " to " << xmv_value << std::endl;
			xmv_pair *mem = static_cast<xmv_pair*>(reg_xmv.get_address());
			*mem = xmv_update;
		}

		if (set_idv  || unset_idv)
		{
			shared_memory_object idv_shm(open_only, IDV_SHMEM_NAME, read_write);
			mapped_region reg_idv(idv_shm, read_write);

			std::cout << "setting idv " << idv_index << std::endl;
			idv_pair *mem = static_cast<idv_pair*>(reg_idv.get_address());
			idv_pair idv_update;
			idv_update.index = idv_index;
			idv_update.value = set_idv ? 1 : 0;
			*mem = idv_update;
		}

		if (print_all)
		{
			shared_memory_object xmeas_shm(open_only, SIM_SHMEM_NAME, read_write);
			mapped_region reg_proc_vars(xmeas_shm, read_write);
			double *mem = static_cast<double*>(reg_proc_vars.get_address());
			for (unsigned ii = 0; ii < (reg_proc_vars.get_size() / sizeof(double)) - 1; ii++)
			{
				std::cout << mem[ii] << ", ";
			}
			std::cout << mem[(reg_proc_vars.get_size() / sizeof(double))-1] << std::endl;
		}

	}
	catch (interprocess_exception & ex)
	{
		std::cout << ex.what() << std::endl;
		return 1;
	}

	return 0;
}



