/*
 * Copyright (C) mavlab
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file "modules/qpoas/qpoas.c"
 * @author mavlab
 * opti
 */

#include "modules/qpoas/qpoas.h"


// Eigen headers
#pragma GCC diagnostic ignored "-Wint-in-bool-context"
#pragma GCC diagnostic ignored "-Wshadow"
#include <Eigen/Dense>
#pragma GCC diagnostic pop
#include <INCLUDE/QProblemB.hpp>



#define MAX_N 100
using namespace Eigen;
USING_NAMESPACE_QPOASES

void qp_init(void) {
float pos0[2] = {10, 2};
	float posf[2] = {0 ,0};
	
	float vel0[2] = {0, 0};
	float velf[2] = {0, 0};
	float dt = 0.1;
	float T = sqrt(pow((pos0[0] - posf[0]),2) + pow((pos0[1] - posf[1]),2)) / 4.0;
	unsigned int N = round(T/dt);
	  
	
	Eigen::Matrix<double, 4, 4> A;
	A << 0.9512, 0, 0, 0,
		0.09754, 1, 0, 0,
		0, 0, 0.9512, 0,
		0, 0, 0.09754, 1;

	Eigen::Matrix<double, 4, 2> B;
	B << 0.9569, 0,
		0.04824, 0,
		0, 0.9569,
		0, 0.04824;

	Eigen::Matrix<double, 4, 4> P;
	P << 1,0,0,0,
		0,10,0,0,
		0,0,1,0,
		0,0,0,10;
	
	// Eigen::Matrix<double, 4, Dynamic> R;
	Eigen::MatrixXd oldR(4, 2 * MAX_N);
	oldR.resize(4, 2*N);
	
	oldR.block(0, 2*N-2, 4, 2) =  B;
	Eigen::Matrix<double, 4, 4> AN = A;

	for(int i=1; i<N; i++) {
		oldR.block(0, 2*N-2*(i+1), 4, 2) =  A * oldR.block(0, 2*N-2*i, 4, 2);
		AN = A * AN; 
	}
	Eigen::MatrixXd R = oldR.block(0,0,4,2*N);

	Eigen::MatrixXd old_H = 2 * (R.transpose() * P * R);
	
	Eigen::MatrixXd eye(2* MAX_N, 2 * MAX_N); 
	eye.resize(2*N, 2*N);
	eye.setIdentity();
	Eigen::MatrixXd H = 0.5 * (old_H + old_H.transpose() + eye);

	Eigen::Matrix<double, 4, 1> x0; Eigen::Matrix<double, 4, 1> xd;
	x0 << vel0[0], pos0[0], vel0[1], pos0[1];
	xd << velf[0], posf[0], velf[1], posf[1];
	
	Eigen::MatrixXd f;
	f = (2 * ((AN * x0)- xd)).transpose() * P * R;
	
	// to compare with matlab
	// cout << "size of H: " << H.rows() << " x " << H.cols() << endl; 
	// cout << "H \n" << H << endl;
	// cout << "size of f: " << f.rows() << " x " << f.cols() << endl; 
	// cout << "f \n" << f << endl;

	float maxbank = 45.0 * 3.142 / 180.0;

	int sizes = 2 * N;

	real_t ub[sizes];
	real_t lb[sizes];
	
	for (int i=0; i<sizes; i++) {
		ub[i] = maxbank;
		lb[i] = -maxbank;
	}
	
	real_t newH[sizes * sizes];
	real_t newf[sizes];

  Eigen::Map<MatrixXd>(newH, sizes, sizes) =   H.transpose();
	Eigen::Map<MatrixXd>(newf, 1, sizes) = f;
	/* to check row or column major 
	for (int i=0; i<sizes*sizes; i++) {
		cout << newH[i] << ",";
	}
	*/
	//to check row or column major 
	// for (int i=0; i<sizes; i++) {
	// 	cout << newf[i] << ",";
	// }
	

	// /* Setting up QProblemB object. */
	QProblemB mpctry( 2*N );  // our class of problem, we don't have any constraints on position or velocity, just the inputs
	// // constructor can be initialized by Hessian type, so it can stop checking for positive definiteness

	// Options optionsmpc;
	// //options.enableFlippingBounds = BT_FALSE;
	// optionsmpc.printLevel = PL_DEBUG_ITER;
	// optionsmpc.initialStatusBounds = ST_INACTIVE;
	// optionsmpc.numRefinementSteps = 1;
	// // optionsmpc.enableCholeskyRefactorisation = 1;
	// mpctry.setOptions( optionsmpc );

	// /* Solve first QP. */
	// int_t nWSR = 100;
	// mpctry.init(newH,newf, lb,ub, nWSR,0);

	// /* Get and print solution of first QP. */
	// real_t xOpt[sizes];
	// mpctry.getPrimalSolution( xOpt );
	// // cout << "U: " << endl;
	// //to check row or column major 
	// for (int i=0; i<sizes; i++) {
	// 	// cout << xOpt[i] << ",";
	// }
	
	// printf( "\nfval = %e\n\n", xOpt[0],xOpt[1],mpctry.getObjVal() );
}

void replan(void) {}


