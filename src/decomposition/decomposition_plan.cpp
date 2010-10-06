/*
 *  DecompositionPlan.cpp
 *
 *  Created on: Dec 30, 2008
 *  Authors:
 *  Benjamin Lindner, ben@benlabs.net
 *
 *  Copyright 2008,2009 Benjamin Lindner
 *
 */

// direct header
#include "decomposition/decomposition_plan.hpp"

// standard header
#include <complex>
#include <fstream>

// special library headers
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/lexical_cast.hpp>

// other headers
#include "math/coor3d.hpp"
#include "decomposition/decompose.hpp"
#include "control.hpp"
#include "log.hpp"

using namespace std;

DecompositionParameters::DecompositionParameters(size_t NN,size_t NQ,size_t NAF,size_t NNpP,size_t elbytesize) {
    size_t NP = NN/NNpP;
    size_t NPused = NP;
    if (NQ<NP) NPused = NQ;
    size_t NNnotused = NN - NPused*NNpP;
    
    size_t NQcycles;
    if ((NQ%NP)==0) {
        NQcycles = NQ/NP;
    } else {
        NQcycles = NQ/NP + 1;
    }
    
    size_t NAFcycles;
    if ((NAF%NNpP)==0) {
        NAFcycles = NAF/NNpP;
    } else {
        NAFcycles =  NAF/NNpP + 1;
    }
    
    size_t penalty = NNnotused*NQcycles*NAFcycles;
    penalty +=  (NPused*NQcycles - NQ)*(NNpP*NAFcycles);
    penalty += (NNpP*NAFcycles - NAF)*NQ;
    
    // store for future reference
    m_penalty = penalty;
    m_NAFcycles = NAFcycles;
    m_NQcycles = NQcycles;
    m_NNpP = NNpP;

    m_NN = NN;
    m_NQ = NQ;
    m_NAF = NAF;
    m_NP = NP;

    m_elbytesize = elbytesize;
    m_nbytesize = m_NAF*m_elbytesize;
}


DecompositionPlan::DecompositionPlan(size_t nn,size_t nq,size_t naf,size_t elbytesize,size_t nmaxbytesize) {

    // initialize pointer
    p_dp_best = NULL;
    
    if (naf<1) {
        Err::Inst()->write("No data to decompose.");
        throw;
    }
    
    size_t npmax = nn;
    if (npmax>nq) npmax = nq;
        
	if (Params::Inst()->limits.decomposition.partitions.automatic) {
        size_t npmax = naf;
        if (naf>nn) npmax = nn;
        for (size_t nnpp=npmax;nnpp>=1;nnpp--) {
            DecompositionParameters* p_dp = new DecompositionParameters(nn,nq,naf,nnpp,elbytesize);
            if (p_dp->nbytesize()>nmaxbytesize) continue;
            if (p_dp_best == NULL) {
                p_dp_best = p_dp;
            } else {
                if (p_dp->penalty()<p_dp_best->penalty()) {
                    delete p_dp_best;
                    p_dp_best = p_dp;
                } else {
                    delete p_dp;
                }
            }
        } 
    } else {
        if (Params::Inst()->limits.decomposition.partitions.size<=naf) {
            p_dp_best = new DecompositionParameters(nn,nq,naf,Params::Inst()->limits.decomposition.partitions.size,elbytesize);            
        }
    }

    if (p_dp_best == NULL) {
		Err::Inst()->write("No decomposition found to match the necessary requirements.");
		Err::Inst()->write("Either change the partition size manually or change the number of nodes.");
		Err::Inst()->write("Beware that size of a partition <= frames / atoms (depends)");
		throw;
    }
    
    Info::Inst()->write("Final decomposition parameters:");
    Info::Inst()->write(string("NN                : ")+boost::lexical_cast<string>(p_dp_best->get_NN()));
    Info::Inst()->write(string("NQ                : ")+boost::lexical_cast<string>(p_dp_best->get_NQ()));
    Info::Inst()->write(string("NAF               : ")+boost::lexical_cast<string>(p_dp_best->get_NAF()));
    Info::Inst()->write(string("NP                : ")+boost::lexical_cast<string>(p_dp_best->get_NP()));
    Info::Inst()->write(string("NNpP              : ")+boost::lexical_cast<string>(p_dp_best->get_NNpP()));
    Info::Inst()->write(string("NAFcycles         : ")+boost::lexical_cast<string>(p_dp_best->get_NAFcycles()));
    Info::Inst()->write(string("NQcycles          : ")+boost::lexical_cast<string>(p_dp_best->get_NQcycles()));
    size_t used =  (p_dp_best->get_NQ()*p_dp_best->get_NAF());
    size_t wasted = p_dp_best->penalty();
    double utilization = (used)*1.0/(used+wasted);
    Info::Inst()->write(string("CompEl (WASTE/USE): ")+boost::lexical_cast<string>(wasted)+string("/")+boost::lexical_cast<string>(used));
    Info::Inst()->write(string("utilization(1=best)    : ")+boost::lexical_cast<string>(utilization));

    if (utilization < Params::Inst()->limits.decomposition.utilization) {
		Err::Inst()->write(string("Utilization too low. Aborting. Change the number of nodes or the threshold value (")+boost::lexical_cast<string>(Params::Inst()->limits.decomposition.utilization)+string(")"));
        delete p_dp_best;
        p_dp_best = NULL;
		throw;
    }
    
}

DecompositionPlan::~DecompositionPlan() {
    if (p_dp_best!=NULL) delete p_dp_best;
}

size_t DecompositionParameters::PID(size_t rank) {
    return (rank / m_NNpP);
}

size_t DecompositionParameters::penalty() {
    return (m_penalty);
}

double DecompositionParameters::penalty_percent() {
    return (m_penalty / (1.0*m_NAF*m_NQ));
}

std::vector<size_t> DecompositionPlan::colors() {
		
	std::vector<size_t> colors;
    size_t NN = p_dp_best->get_NN();
    for(size_t i=0;i<NN;i++) {
		colors.push_back( p_dp_best->PID(i) );
	}
	return colors;
}

double DecompositionPlan::static_imbalance() {
    return p_dp_best->penalty_percent();
}

size_t DecompositionPlan::partitions() {
	return p_dp_best->get_NP();
}

size_t DecompositionPlan::partitionsize() {
	return p_dp_best->get_NNpP();
}

size_t DecompositionPlan::nbytesize() {
    return p_dp_best->nbytesize();
}

// end of file
