// --------------------------------------------------------------------- //
// For part B, you will need to modify this file.                        //
// You may add any code you need, as long as you correctly implement the //
// three required BPred methods already listed in this file.             //
// --------------------------------------------------------------------- //

// bpred.cpp
// Implements the branch predictor class.

#include "bpred.h"

/**
 * Construct a branch predictor with the given policy.
 * 
 * In part B of the lab, you must implement this constructor.
 * 
 * @param policy the policy this branch predictor should use
 */
BPred::BPred(BPredPolicy policy)
{
    this->policy = policy;
    this->stat_num_branches = 0;
    this->stat_num_mispred = 0;
    this->ghr = 0;

    //Initializing all PHT entries to wealk taken (2'b10)
    for(int i=0; i<4096; i++){
        this->pht[i] = 2;
    }
}

/**
 * Get a prediction for the branch with the given address.
 * 
 * In part B of the lab, you must implement this method.
 * 
 * @param pc the address (program counter) of the branch to predict
 * @return the prediction for whether the branch is taken or not taken
 */
BranchDirection BPred::predict(uint64_t pc)
{
    if (policy == BPRED_ALWAYS_TAKEN)
    {
        return TAKEN;
    }

    if(policy == BPRED_GSHARE){
        //Index 
        uint32_t index = (pc ^ ghr) & 0xFFF;

        //Prediction
        uint8_t prediction = pht[index];

        if(prediction >= 2) return TAKEN;    
        else return NOT_TAKEN; 
    }

    return TAKEN;
}


/**
 * Update the branch predictor statistics (stat_num_branches and
 * stat_num_mispred), as well as any other internal state you may need to
 * update in the branch predictor.
 * 
 * In part B of the lab, you must implement this method.
 * 
 * @param pc the address (program counter) of the branch
 * @param prediction the prediction made by the branch predictor
 * @param resolution the actual outcome of the branch
 */
void BPred::update(uint64_t pc, BranchDirection prediction,
                   BranchDirection resolution)
{
    stat_num_branches++;

    if (prediction != resolution)
    {
        stat_num_mispred++;
    }

    if (policy == BPRED_ALWAYS_TAKEN)
    {
        return;
    }

    if(policy == BPRED_GSHARE){
        uint32_t index = (pc ^ ghr) & 0xFFF;

        if(resolution == TAKEN){
            pht[index] = sat_increment(pht[index], 3);
        }
        else{
            pht[index] = sat_decrement(pht[index]);
        }

        // Shift the branch resolution into GHR 
        ghr = ((ghr << 1) | (resolution == TAKEN ? 1 : 0)) & 0xFFF;
    }
}
