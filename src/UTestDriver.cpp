#include "Rcpp.h"
#include "UGenVar.h"
#include "CoefGen.h"
#include "PhiGen.h"
#include "WGen.h"
#include "XiGen.h"

#include "cppunit/ui/text/TestRunner.h"
#include "UTestFactory.h"
#include "UTestGammaCateg.h"
#include "UTestGammaContMH.h"
#include "UTestPhiGen.h"
#include "UTestUGenVarCateg.h"
#include "UTestWGen.h"
#include "UTestXGen.h"
#include "UTestXiGen.h"

extern int* d2s;

UTestFactory g_ut_factory;

// preg_cycle            used when sampling W
// w_to_days_idx         categorical gamma: a_tilde
// w_cyc_to_subj_idx     used when sampling xi (first term)
// fw_len                how much memory to set aside when sampling W in a cycle
// subj_day_block        used when sampling xi (second term)
// gamma_specs           gamma hyperparameters
// phi_specs             phi hyperparameters




// [[Rcpp::export]]
int utest_cpp_(Rcpp::NumericMatrix u_rcpp,
	       Rcpp::IntegerVector x_rcpp,
	       Rcpp::List          w_day_blocks,
	       Rcpp::IntegerVector w_to_days_idx,
	       Rcpp::IntegerVector w_cyc_to_subj_idx,
	       Rcpp::List          subj_day_blocks,
	       Rcpp::IntegerVector day_to_subj_idx,
	       Rcpp::List          gamma_specs,
	       Rcpp::NumericVector phi_specs,
	       Rcpp::List          x_miss_cyc,
	       Rcpp::List          x_miss_day,
	       Rcpp::NumericVector utau_rcpp,
	       Rcpp::List          tau_coefs,
	       Rcpp::List          u_miss_info,
	       Rcpp::IntegerVector u_miss_type,
	       Rcpp::IntegerVector u_preg_map,
	       Rcpp::IntegerVector u_sex_map,
	       int fw_len,
	       int n_burn,
	       int n_samp,
	       Rcpp::List test_data) {

    // so that data generation classes record their samples.  By default this
    // value is false.
    g_record_status = true;

    UTestFactory::epsilon = Rcpp::as<double>(test_data["epsilon"]);
    g_ut_factory = UTestFactory(u_rcpp,
    				x_rcpp,
    				w_day_blocks,
    				w_to_days_idx,
    				w_cyc_to_subj_idx,
    				subj_day_blocks,
    				day_to_subj_idx,
    				gamma_specs,
    				phi_specs,
				x_miss_cyc,
				x_miss_day,
				utau_rcpp,
				tau_coefs,
				u_miss_info,
				u_miss_type,
				u_preg_map,
				u_sex_map,
    				fw_len,
    				n_burn,
    				n_samp,
    				test_data);

    d2s = day_to_subj_idx.begin();

    CppUnit::TextUi::TestRunner runner;
    runner.addTest(GammaCategTest::suite());
    runner.addTest(GammaContMHTest::suite());
    runner.addTest(PhiGenTest::suite());
    if (u_miss_info.size() > 0) { runner.addTest(UGenVarCategTest::suite()); }
    runner.addTest(WGenTest::suite());
    if (x_miss_cyc.size() > 0) { runner.addTest(XGenTest::suite()); }
    runner.addTest(XiGenTest::suite());

    // run() returns true if successful, false otherwise
    return runner.run() ? 0 : 1;
}
