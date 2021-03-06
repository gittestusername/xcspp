#pragma once
#include <iosfwd> // std::ostream
#include <vector>
#include <cstdint> // std::uint64_t
#include <cstddef> // std::size_t

#include "xcspp/core/iclassifier_system.hpp"
#include "xcsr_params.hpp"
#include "population.hpp"
#include "action_set.hpp"
#include "prediction_array.hpp"

namespace xcspp::xcsr
{

    class XCSR : public IRealClassifierSystem
    {
    private:
        // Random utility instance
        Random m_random;

        // Hyperparameters
        XCSRParams m_params;

        // [P]
        //   The population [P] consists of all classifier that exist in XCS at any time.
        Population m_population;

        // [A]
        //   The action set [A] is formed out of the current [M].
        //   It includes all classifiers of [M] that propose the executed action.
        ActionSet m_actionSet;

        // [A]_-1
        //   The previous action set [A]_-1 is the action set that was active in the last
        //   execution cycle.
        ActionSet m_prevActionSet;

        // Available action choices
        const std::unordered_set<int> m_availableActions;

        std::uint64_t m_timeStamp;

        bool m_expectsReward;
        double m_prevReward;
        bool m_isPrevModeExplore;

        std::vector<double> m_prevSituation;

        // Prediction value of the previous action decision (just for logging)
        double m_prediction;
        std::unordered_map<int, double> m_predictions;

        // Covering occurrence of the previous action decision (just for logging)
        bool m_isCoveringPerformed;

        // Set system timestamp to the same as the latest classifier in [P]
        void syncTimeStampWithPopulation();

    public:
        // Constructor
        XCSR(const std::unordered_set<int> & availableActions, const XCSRParams & params);

        // Destructor
        ~XCSR() = default;

        // Run with exploration
        int explore(const std::vector<double> & situation);

        // Feedback reward to system
        void reward(double value, bool isEndOfProblem = true);

        // Run without exploration
        // (Set update to true when testing multi-step problems. If update is true, make sure to call reward() after this.)
        int exploit(const std::vector<double> & situation, bool update = false);

        // Get prediction value of the previous action decision
        // (Call this function after explore() or exploit())
        double prediction() const;

        // Get prediction value of the action
        // (Call this function after explore() or exploit())
        double predictionFor(int action) const;

        // Get if covering is performed in the previous action decision
        // (Call this function after explore() or exploit())
        bool isCoveringPerformed() const;

        // Get all classifiers that match the given situation
        std::vector<Classifier> getMatchingClassifiers(const std::vector<double> & situation) const;

        // Get const reference to population
        const Population & population() const;

        void setPopulationClassifiers(const std::vector<Classifier> & classifiers, bool syncTimeStamp = true);

        [[deprecated("use XCS::outputPopulationCSV() instead")]]
        void dumpPopulation(std::ostream & os) const;

        void outputPopulationCSV(std::ostream & os) const;

        bool loadPopulationCSVFile(const std::string & filename, bool initClassifierVariables = false, bool syncTimeStamp = true);

        bool savePopulationCSVFile(const std::string & filename) const;

        std::size_t populationSize() const;

        std::size_t numerositySum() const;

        void switchToCondensationMode();
    };

}
