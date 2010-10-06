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

#ifndef SCATTER_DEVICES__ALL__VECTORSTHREAD_HPP_
#define SCATTER_DEVICES__ALL__VECTORSTHREAD_HPP_

// common header
#include "common.hpp"

// standard header
#include <complex>
#include <map>
#include <string>
#include <sys/time.h>
#include <vector>
#include <queue>

// special library headers
#include <boost/asio.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/mpi.hpp>
#include <boost/thread.hpp>

// other headers
#include "sample.hpp"
#include "services.hpp"
#include "math/coor3d.hpp"
#include "scatter_devices/scatter_factors.hpp"
#include "report/timer.hpp"

#include "scatter_devices/scatter_device.hpp"

class AllVectorsThreadScatterDevice : public ScatterDevice {
protected:
	boost::mpi::communicator m_scattercomm;
	boost::mpi::communicator m_fqtcomm;
	Sample* p_sample;

	std::vector<std::pair<size_t,CartesianCoor3D> > m_qvectorindexpairs;
	size_t m_current_qvector;
    
    HDF5WriterClient* p_hdf5writer;
    MonitorClient* p_monitor;

    size_t NN,NF,NA,NM;

    // first = q, second = frames
    concurrent_queue< size_t > at0;    
    concurrent_queue< std::pair<size_t,std::vector< std::complex<double> >* > > at1;
    concurrent_queue< std::vector< std::complex<double> >* > at2;
    
    mutable boost::mutex at3_mutex;
    mutable std::vector< std::complex<double> > at3;
	
	// data, outer loop by frame, inner by atoms, XYZ entries
    coor_t* p_coordinates;
    
	ScatterFactors scatterfactors;
	std::vector<size_t> myframes;
		
	std::vector<std::complex<double> > m_spectrum;		
        
    std::vector<CartesianCoor3D> qvectors;
    
	// have to be implemented by concrete classes:
    void init(CartesianCoor3D& q);	
	void scatter(size_t moffset);
    
    void stage_data();
        
    void worker1(bool loop);
    void worker2(bool loop);
    void worker3(bool loop);
    
    volatile size_t worker2_counter;
    mutable boost::mutex worker3_mutex;
    mutable boost::mutex worker2_mutex;    
    volatile bool worker2_done;
    volatile bool worker3_done;
    boost::condition_variable worker3_notifier;
    boost::condition_variable worker2_notifier;
    
    std::queue<boost::thread*> worker_threads;
    
	void compute(bool marshal);
	void next();
	void write();
	
    void runner();
    
    void start_workers();
    void stop_workers();
    
public: 
    AllVectorsThreadScatterDevice(
			boost::mpi::communicator scatter_comm,
			boost::mpi::communicator fqt_comm,
			Sample& sample,
			std::vector<std::pair<size_t,CartesianCoor3D> > QVI,
			std::vector<size_t> assignment,
			boost::asio::ip::tcp::endpoint fileservice_endpoint,
			boost::asio::ip::tcp::endpoint monitorservice_endpoint			

	);
    virtual ~AllVectorsThreadScatterDevice();


	size_t status();
    double_t progress();
    
    void run();
};


#endif

//end of file
