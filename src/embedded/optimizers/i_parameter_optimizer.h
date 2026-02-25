// src/embedded/optimizers/i_parameter_optimizer.h
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "../workflow_controller.h"

namespace roboclaw::embedded {

/**
 * @brief Plant model for control system optimization
 */
struct PlantModel {
    std::string model_type;      // "first_order", "second_order", "integrator"
    double gain = 1.0;          // Process gain
    double time_constant = 1.0;  // Time constant (tau)
    double delay = 0.0;          // Transport delay
    nlohmann::json additional_params;
};

/**
 * @brief Optimization result structure
 */
struct OptimizationResult {
    bool success;
    nlohmann::json parameters;    // Optimized parameter values
    double final_cost;            // Final optimization cost
    int iterations;               // Number of iterations performed
    std::string method_used;      // Optimization method used
    std::vector<double> cost_history;  // Cost per iteration
};

/**
 * @brief Interface for parameter optimizers
 *
 * Parameter optimizers tune control system parameters (PID gains, etc.)
 * based on plant model and test data.
 */
class IParameterOptimizer {
public:
    virtual ~IParameterOptimizer() = default;

    /**
     * @brief Get optimizer name
     * @return Optimizer name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Optimize parameters
     * @param plant Plant model
     * @param current_params Current parameter values
     * @param constraints Optimization constraints
     * @return Optimization result
     */
    virtual OptimizationResult optimize(
        const PlantModel& plant,
        const nlohmann::json& current_params,
        const OptimizationConstraints& constraints
    ) = 0;

    /**
     * @brief Validate parameters are within safe ranges
     * @param params Parameters to validate
     * @param constraints Constraints to check against
     * @return true if parameters are valid
     */
    virtual bool validateParameters(const nlohmann::json& params,
                                   const OptimizationConstraints& constraints) const;

    /**
     * @brief Calculate cost function value for parameters
     * @param plant Plant model
     * @param params Parameter values to evaluate
     * @param constraints Constraints for weighting
     * @return Cost value (lower is better)
     */
    virtual double calculateCost(const PlantModel& plant,
                                const nlohmann::json& params,
                                const OptimizationConstraints& constraints) const;

protected:
    // Helper functions for cost calculation
    static double estimateOvershoot(double kp, double ki, double kd, const PlantModel& plant);
    static double estimateSettlingTime(double kp, double ki, double kd, const PlantModel& plant);
};

/**
 * @brief Ziegler-Nichols PID tuner (rule-based method)
 *
 * Classic Ziegler-Nichols method for PID tuning.
 * Fast but may not work well for all systems.
 */
class ZieglerNicholsOptimizer : public IParameterOptimizer {
public:
    std::string getName() const override { return "Ziegler-Nichols"; }

    OptimizationResult optimize(
        const PlantModel& plant,
        const nlohmann::json& current_params,
        const OptimizationConstraints& constraints
    ) override;
};

/**
 * @brief Genetic Algorithm optimizer (evolutionary method)
 *
 * Population-based optimization that can handle complex,
 * non-linear systems. Slower but more robust.
 */
class GeneticAlgorithmOptimizer : public IParameterOptimizer {
public:
    struct GAConfig {
        int population_size = 50;
        int generations = 100;
        double mutation_rate = 0.1;
        double crossover_rate = 0.8;
        int elite_count = 2;
    };

    GeneticAlgorithmOptimizer();
    explicit GeneticAlgorithmOptimizer(const GAConfig& config);

    std::string getName() const override { return "Genetic Algorithm"; }

    OptimizationResult optimize(
        const PlantModel& plant,
        const nlohmann::json& current_params,
        const OptimizationConstraints& constraints
    ) override;

    void setConfig(const GAConfig& config) { config_ = config; }

private:
    GAConfig config_;

    struct Individual {
        nlohmann::json genes;
        double fitness;
    };

    std::vector<Individual> initializePopulation(int size,
                                                 const nlohmann::json& bounds);
    double evaluateFitness(const Individual& individual,
                          const PlantModel& plant,
                          const OptimizationConstraints& constraints);
    std::vector<Individual> selection(const std::vector<Individual>& population);
    Individual crossover(const Individual& parent1, const Individual& parent2);
    void mutate(Individual& individual, const nlohmann::json& bounds);
};

/**
 * @brief Bayesian Optimization (adaptive method)
 *
 * Efficient optimization for expensive-to-evaluate functions.
 * Uses Gaussian Process regression to model the cost surface.
 */
class BayesianOptimizer : public IParameterOptimizer {
public:
    struct BayesConfig {
        int iterations = 50;
        int initial_samples = 10;
        double exploration_weight = 0.5;  // 0 = pure exploitation, 1 = pure exploration
    };

    BayesianOptimizer();
    explicit BayesianOptimizer(const BayesConfig& config);

    std::string getName() const override { return "Bayesian Optimization"; }

    OptimizationResult optimize(
        const PlantModel& plant,
        const nlohmann::json& current_params,
        const OptimizationConstraints& constraints
    ) override;

    void setConfig(const BayesConfig& config) { config_ = config; }

private:
    BayesConfig config_;
    std::vector<std::pair<nlohmann::json, double>> sample_history_;
};

/**
 * @brief Optimizer registry for managing available optimizers
 */
class OptimizerRegistry {
public:
    static OptimizerRegistry& instance();

    void registerOptimizer(const std::string& name,
                         std::shared_ptr<IParameterOptimizer> optimizer);

    std::shared_ptr<IParameterOptimizer> getOptimizer(const std::string& name);

    std::vector<std::string> listOptimizers() const;

private:
    OptimizerRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<IParameterOptimizer>> optimizers_;
};

} // namespace roboclaw::embedded
