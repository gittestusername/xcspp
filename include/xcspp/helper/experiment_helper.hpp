#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <functional>
#include <unordered_set>
#include <cstdio>
#include <cstddef>
#include <cmath>
#include "xcspp/xcs.hpp"
#include "xcspp/environment/ienvironment.hpp"
#include "experiment_settings.hpp"
#include "experiment_log_stream.hpp"

namespace xcspp
{

    class ExperimentHelper
    {
    private:
        const ExperimentSettings m_settings;
        const XCSParams m_constants;
        std::vector<std::unique_ptr<XCS>> m_experiments;
        std::vector<std::unique_ptr<IEnvironment>> m_explorationEnvironments;
        std::vector<std::unique_ptr<IEnvironment>> m_exploitationEnvironments;
        std::function<void(IEnvironment &)> m_explorationCallback;
        std::function<void(IEnvironment &)> m_exploitationCallback;
        std::ofstream m_summaryLogStream;
        bool m_outputSummaryLogFile;
        SMAExperimentLogStream m_rewardLogStream;
        SMAExperimentLogStream m_systemErrorLogStream;
        SMAExperimentLogStream m_stepCountLogStream;
        ExperimentLogStream m_populationSizeLogStream;
        bool m_alreadyOutputSummaryHeader;
        double m_summaryRewardSum;
        double m_summarySystemErrorSum;
        double m_summaryPopulationSizeSum;
        double m_summaryCoveringOccurrenceRateSum;
        double m_summaryStepCountSum;
        std::size_t m_iterationCount;

        static std::vector<std::unique_ptr<XCS>> makeExperiments(
            const ExperimentSettings & settings,
            const std::unordered_set<int> & availableActions,
            const XCSParams & constants)
        {
            std::vector<std::unique_ptr<XCS>> experiments;
            for (std::size_t i = 0; i < settings.seedCount; ++i)
            {
                experiments.push_back(
                    std::make_unique<XCS>(availableActions, constants)
                );
            }
            return experiments;
        }

    public:
        ExperimentHelper(const ExperimentSettings & settings, const XCSParams & constants)
            : m_settings(settings)
            , m_constants(constants)
            , m_explorationCallback(nullptr)
            , m_exploitationCallback(nullptr)
            , m_summaryLogStream(settings.outputSummaryFilename.empty() ? "" : (settings.outputFilenamePrefix + settings.outputSummaryFilename))
            , m_rewardLogStream(settings.outputRewardFilename.empty() ? "" : (settings.outputFilenamePrefix + settings.outputRewardFilename), settings.smaWidth, false)
            , m_systemErrorLogStream(settings.outputSystemErrorFilename.empty() ? "" : (settings.outputFilenamePrefix + settings.outputSystemErrorFilename), settings.smaWidth, false)
            , m_stepCountLogStream(settings.outputStepCountFilename.empty() ? "" : (settings.outputFilenamePrefix + settings.outputStepCountFilename), settings.smaWidth, false)
            , m_populationSizeLogStream(settings.outputPopulationSizeFilename.empty() ? "" : (settings.outputFilenamePrefix + settings.outputPopulationSizeFilename), false)
            , m_alreadyOutputSummaryHeader(false)
            , m_summaryRewardSum(0.0)
            , m_summarySystemErrorSum(0.0)
            , m_summaryPopulationSizeSum(0.0)
            , m_summaryCoveringOccurrenceRateSum(0.0)
            , m_summaryStepCountSum(0.0)
            , m_iterationCount(0)
        {
            if (!settings.inputClassifierFilename.empty())
            {
                for (const auto & experiment : m_experiments)
                {
                    experiment->loadPopulationCSV(settings.inputClassifierFilename, !settings.useInputClassifierToResume);
                }
            }
        }

        ~ExperimentHelper() = default;

        template <class Environment, class... Args>
        void constructEnvironments(Args && ... args)
        {
            // Clear environment arrays
            m_explorationEnvironments.clear();
            m_exploitationEnvironments.clear();

            if (m_settings.seedCount == 0)
            {
                return;
            }

            // Construct environments
            for (std::size_t i = 0; i < m_settings.seedCount; ++i)
            {
                m_explorationEnvironments.push_back(std::make_unique<Environment>(std::forward<Args>(args)...));
                m_exploitationEnvironments.push_back(std::make_unique<Environment>(std::forward<Args>(args)...));
            }

            // Construct experiments
            m_experiments = makeExperiments(m_settings, m_explorationEnvironments.at(0)->availableActions(), m_constants);
        }

        void runIteration(std::size_t repeat = 1)
        {
            for (std::size_t i = 0; i < repeat; ++i)
            {
                // Exploitation
                if (m_settings.exploitationCount > 0)
                {
                    std::size_t totalStepCount = 0;
                    double rewardSum = 0.0;
                    double systemErrorSum = 0.0;
                    double populationSizeSum = 0.0;
                    for (std::size_t j = 0; j < m_settings.seedCount; ++j)
                    {
                        for (std::size_t k = 0; k < m_settings.exploitationCount; ++k)
                        {
                            do
                            {
                                // Choose action
                                auto action = m_experiments[j]->exploit(m_exploitationEnvironments[j]->situation(), m_settings.updateInExploitation);

                                // Get reward
                                double reward = m_exploitationEnvironments[j]->executeAction(action);
                                m_summaryRewardSum += reward / m_settings.exploitationCount / m_settings.seedCount;
                                m_summarySystemErrorSum += std::abs(reward - m_experiments[j]->prediction()) / m_settings.exploitationCount / m_settings.seedCount;
                                m_summaryCoveringOccurrenceRateSum += static_cast<double>(m_experiments[j]->isCoveringPerformed()) / m_settings.exploitationCount / m_settings.seedCount;
                                if (m_settings.updateInExploitation)
                                {
                                    m_experiments[j]->reward(reward, m_exploitationEnvironments[j]->isEndOfProblem());
                                }
                                rewardSum += reward;
                                systemErrorSum += std::abs(reward - m_experiments[j]->prediction());
                                ++totalStepCount;

                                // Run callback if needed
                                if (m_exploitationCallback != nullptr)
                                {
                                    m_exploitationCallback(*m_exploitationEnvironments[j]);
                                }
                            } while (!m_exploitationEnvironments[j]->isEndOfProblem());

                            populationSizeSum += m_experiments[j]->populationSize();
                        }
                        m_summaryPopulationSizeSum += static_cast<double>(m_experiments[j]->populationSize()) / m_settings.seedCount;
                    }

                    m_summaryStepCountSum += static_cast<double>(totalStepCount) / m_settings.exploitationCount / m_settings.seedCount;

                    if (m_settings.summaryInterval > 0 && (m_iterationCount + 1) % m_settings.summaryInterval == 0)
                    {
                        if (!m_alreadyOutputSummaryHeader)
                        {
                            if (m_settings.outputSummaryToStdout)
                            {
                                std::cout
                                    << "  Iteration      Reward      SysErr     PopSize  CovOccRate   TotalStep\n"
                                    << " ========== =========== =========== =========== =========== ===========" << std::endl;
                            }
                            if (m_summaryLogStream)
                            {
                                m_summaryLogStream << "Iteration,Reward,SysErr,PopSize,CovOccRate,TotalStep" << std::endl;
                            }
                            m_alreadyOutputSummaryHeader = true;
                        }
                        if (m_settings.outputSummaryToStdout)
                        {
                            std::printf("%11u %11.3f %11.3f %11.3f  %1.8f %11.3f\n",
                                static_cast<unsigned int>(m_iterationCount + 1),
                                m_summaryRewardSum / m_settings.summaryInterval,
                                m_summarySystemErrorSum / m_settings.summaryInterval,
                                m_summaryPopulationSizeSum / m_settings.summaryInterval,
                                m_summaryCoveringOccurrenceRateSum / m_settings.summaryInterval,
                                m_summaryStepCountSum / m_settings.summaryInterval);
                            std::fflush(stdout);
                        }
                        if (m_summaryLogStream)
                        {
                            m_summaryLogStream
                                << (m_iterationCount + 1) << ','
                                << m_summaryRewardSum / m_settings.summaryInterval << ','
                                << m_summarySystemErrorSum / m_settings.summaryInterval << ','
                                << m_summaryPopulationSizeSum / m_settings.summaryInterval << ','
                                << m_summaryCoveringOccurrenceRateSum / m_settings.summaryInterval << ','
                                << m_summaryStepCountSum / m_settings.summaryInterval << std::endl;
                        }
                        m_summaryRewardSum = 0.0;
                        m_summarySystemErrorSum = 0.0;
                        m_summaryPopulationSizeSum = 0.0;
                        m_summaryCoveringOccurrenceRateSum = 0.0;
                        m_summaryStepCountSum = 0.0;
                    }

                    m_rewardLogStream.writeLine(rewardSum / m_settings.exploitationCount / m_settings.seedCount);
                    m_systemErrorLogStream.writeLine(systemErrorSum / m_settings.exploitationCount / m_settings.seedCount);
                    m_populationSizeLogStream.writeLine(populationSizeSum / m_settings.exploitationCount / m_settings.seedCount);
                    m_stepCountLogStream.writeLine(static_cast<double>(totalStepCount) / m_settings.exploitationCount / m_settings.seedCount);
                }

                // Exploration
                for (std::size_t j = 0; j < m_settings.seedCount; ++j)
                {
                    for (std::size_t k = 0; k < m_settings.explorationCount; ++k)
                    {
                        do
                        {
                            // Get situation from environment
                            auto situation = m_explorationEnvironments[j]->situation();

                            // Choose action
                            auto action = m_experiments[j]->explore(situation);

                            // Get reward
                            double reward = m_explorationEnvironments[j]->executeAction(action);
                            m_experiments[j]->reward(reward, m_explorationEnvironments[j]->isEndOfProblem());

                            // Run callback if needed
                            if (m_explorationCallback != nullptr)
                            {
                                m_explorationCallback(*m_explorationEnvironments[j]);
                            }
                        } while (!m_explorationEnvironments[j]->isEndOfProblem());
                    }
                }

                ++m_iterationCount;
            }
        }

        void switchToCondensationMode()
        {
            for (const auto & experiment : m_experiments)
            {
                experiment->switchToCondensationMode();
            }
        }

        std::size_t seedCount() const
        {
            return m_settings.seedCount;
        }

        XCS & experimentAt(std::size_t seedIdx)
        {
            return *m_experiments.at(seedIdx);
        }

        const XCS & experimentAt(std::size_t seedIdx) const
        {
            return *m_experiments.at(seedIdx);
        }

        IEnvironment & explorationEnvironmentAt(std::size_t seedIdx)
        {
            return *m_explorationEnvironments.at(seedIdx);
        }

        const IEnvironment & explorationEnvironmentAt(std::size_t seedIdx) const
        {
            return *m_explorationEnvironments.at(seedIdx);
        }

        IEnvironment & exploitationEnvironmentAt(std::size_t seedIdx)
        {
            return *m_exploitationEnvironments.at(seedIdx);
        }

        const IEnvironment & exploitationEnvironmentAt(std::size_t seedIdx) const
        {
            return *m_exploitationEnvironments.at(seedIdx);
        }

        void dumpPopulation(std::size_t seedIdx, std::ostream & os) const
        {
            m_experiments.at(seedIdx)->dumpPopulation(os);
        }
    };

}
