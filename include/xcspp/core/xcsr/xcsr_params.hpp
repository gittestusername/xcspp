#pragma once
#include <memory>
#include <cstdint> // std::uint64_t

#include "xcsr_repr.hpp"
#include "xcspp/util/random.hpp"

namespace xcspp::xcsr
{

    // XCS Hyperparameters
    struct XCSRParams
    {
        // N
        //   The maximum size of the population
        //   (the sum of the classifier numerosities in micro-classifiers)
        //   Recommended: large enough
        std::uint64_t n = 10000;

        // beta
        //   The learning rate for updating fitness, prediction, prediction error, and
        //   action set size estimate in XCS's classifiers
        //   Recommended: 0.1-0.2
        double beta = 0.2;

        // alpha
        //   The fall of rate in the fitness evaluation
        //   Recommended: 0.1
        double alpha = 0.1;

        // epsilon_0
        //   The error threshold under which the accuracy of a classifier is set to one
        //   Recommended: 1% of the maximum reward
        double epsilonZero = 10;

        // nu
        //   The exponent in the power function for the fitness evaluation
        //   Recommended: 5
        double nu = 5;

        // gamma
        //   The discount rate in multi-step problems
        //   Recommended: 0.71
        double gamma = 0.71;

        // theta_GA
        //   The threshold for the GA application in an action set
        //   Recommended: 25-50
        std::uint64_t thetaGA = 25;

        // chi
        //   The probability of applying crossover
        //   Recommended: 0.5-1.0
        double chi = 0.8;

        // crossoverMethod
        enum class CrossoverMethod
        {
            kUniformCrossover,
            kOnePointCrossover,
            kTwoPointCrossover,
        };
        CrossoverMethod crossoverMethod = CrossoverMethod::kUniformCrossover;

        // mu
        //   The probability of mutating one allele and the action
        //   Recommended: 0.01-0.05
        double mu = 0.04;

        // theta_del
        //   The experience threshold over which the fitness of a classifier may be
        //   considered in its deletion probability
        //   Recommended: 20
        std::uint64_t thetaDel = 20;

        // delta
        //   The fraction of the mean fitness of the population below which the fitness
        //   of a classifier may be considered in its vote for deletion
        //   Recommended: 0.1
        double delta = 0.1;

        // theta_sub
        //   The experience of a classifier required to be a subsumer
        //   Recommended: 20
        std::uint64_t thetaSub = 20;

        // tau
        //   The tournament size for selection [Butz et al., 2003]
        //   (set "0" to use the roulette-wheel selection)
        double tau = 0.4;

        // Note: XCSR does not have P_sharp parameter

        // p_I
        //   The initial prediction value when generating a new classifier
        //   Recommended: very small (essentially zero)
        double initialPrediction = 0.01;

        // epsilon_I
        //   The initial prediction error value when generating a new classifier
        //   Recommended: very small (essentially zero)
        double initialEpsilon = 0.01;

        // F_I
        //   The initial fitness value when generating a new classifier
        //   Recommended: very small (essentially zero)
        double initialFitness = 0.01;

        // p_explr
        //   The probability during action selection of choosing the action uniform
        //   randomly
        //   Recommended: 0.5 (depends on the type of experiment)
        double exploreProbability = 1.0;

        // theta_mna
        //   The minimal number of actions that must be present in a match set [M],
        //   or else covering will occur
        //   Recommended: the number of available actions
        //                (or use "0" to set automatically)
        std::uint64_t thetaMna = 0;

        // doGASubsumption
        //   Whether offspring are to be tested for possible logical subsumption by
        //   parents
        bool doGASubsumption = true;

        // doActionSetSubsumption
        //   Whether action sets are to be tested for subsuming classifiers
        bool doActionSetSubsumption = true;

        // doActionMutation
        //   Whether to apply the mutation to the action
        bool doActionMutation = true;

        // useMAM
        //   Whether to use the moyenne adaptive modifee (MAM) for updating the
        //   prediction and the prediction error of classifiers
        bool useMAM = true;

        // ========== XCSR parameters from here ==========

        // s_0
        //   The maximum value of a spread in the covering operator
        double s0 = 0.25;

        // m
        //   The maximum change of a spread value or a center value in the mutation
        double m = 0.1;

        // XCSR representation
        XCSRRepr repr = XCSRRepr::kOBR;

        // The maximum/minimum value of a classifier symbol value
        double minValue = 0.0;
        double maxValue = 1.0;

        // doRangeRestriction (ignored when repr=CSR)
        //   Whether to restrict the range of the condition to the interval
        //   [min-value, max-value)
        bool doRangeRestriction = true;

        // doCoveringRandomRangeTruncation (ignored when repr=CSR)
        //   Whether to truncate the covering random range before generating random
        //   intervals if the interval [x-s_0, x+s_0) is not contained in
        //   [min-value, max-value).
        //   "false" is common for this option, but the covering operator can
        //   generate too many maximum-range intervals if s_0 is larger than
        //   (max-value - min-value) / 2.
        //   Choose "true" to avoid the random bias in this situation.
        bool doCoveringRandomRangeTruncation = false;
    };

}
