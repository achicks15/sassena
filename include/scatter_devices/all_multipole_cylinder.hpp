/*
 *  scatterdevices.hpp
 *
 *  Created on: May 26, 2008
 *  Authors:
 *  Benjamin Lindner, ben@benlabs.net
 *
 *  Copyright 2008,2009 Benjamin Lindner
 *
 */

#ifndef SCATTER_DEVICES__MULTIPOLE_CYLINDER_HPP_
#define SCATTER_DEVICES__MULTIPOLE_CYLINDER_HPP_

// common header
#include "common.hpp"

// standard header
#include <complex>
#include <map>
#include <string>
#include <sys/time.h>
#include <vector>

// special library headers
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/mpi.hpp>

// other headers
#include "sample.hpp"

#include "math/coor3d.hpp"
#include "report/timer.hpp"

#include "scatter_devices/all.hpp"

class AllMCScatterDevice : public AllScatterDevice {
private:
    std::vector<long> moments;
    CartesianCoor3D q;

	// have to be implemented by concrete classes:
    void init(CartesianCoor3D& q);
    size_t get_numberofmoments();	
	void scatter(size_t moffset,size_t mcount);
    void norm();	

public:
	AllMCScatterDevice(boost::mpi::communicator& thisworld, Sample& sample);

};


#endif

//end of file
