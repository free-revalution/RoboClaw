// src/embedded/optimizers/parameter_optimizer.cpp
#include "i_parameter_optimizer.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <limits>

namespace roboclaw::embedded {

//=============================================================================
// IParameterOptimizer - Base class implementations
//=============================================================================

bool IParameterOptimizer::validateParameters(const nlohmann::json& params,
                                           const OptimizationConstraints& constraints) const {
    // Check if required parameters exist
    if (!params.contains("kp") || !params.contains("ki") || !params.contains("kd")) {
        return false;
    }

    double kp = params["kp"].get<double>();
    double ki = params["ki"].get<double>();
    double kd = params["kd"].get<double>();

    // Basic sanity checks
    if (kp < 0 || ki < 0 || kd < 0) {
        return false;
    }

    // Check against parameter ranges if specified
    if (constraints.param_ranges.contains("kp")) {
        auto range = constraints.param_ranges["kp"];
        if (kp < range["min"].get<double>() || kp > range["max"].get<double>()) {
            return false;
        }
    }

    return true;
}

double IParameterOptimizer::calculateCost(const PlantModel& plant,
                                         const nlohmann::json& params,
                                         const OptimizationConstraints& constraints) const {
    // Simple cost function based on expected performance
    // In real implementation, this would run actual tests or simulations

    double kp = params.value("kp", 1.0);
    double ki = params.value("ki", 0.0);
    double kd = params.value("kd", 0.0);

    // Estimate overshoot based on parameters
    double estimated_overshoot = estimateOvershoot(kp, ki, kd, plant);

    // Penalty for overshoot exceeding limit
    double overshoot_penalty = 0.0;
    if (estimated_overshoot > constraints.max_overshoot) {
        overshoot_penalty = 100.0 * (estimated_overshoot - constraints.max_overshoot);
    }

    // Penalty for slow response
    double settling_estimate = estimateSettlingTime(kp, ki, kd, plant);
    double settling_penalty = 0.0;
    if (settling_estimate > constraints.max_settling_time) {
        settling_penalty = 10.0 * (settling_estimate - constraints.max_settling_time);
    }

    // Stability penalty
    double stability_penalty = 0.0;
    if (kp * kd > plant.time_constant * 2.0) {
        stability_penalty = 50.0;
    }

    return overshoot_penalty + settling_penalty + stability_penalty;
}

double IParameterOptimizer::estimateOvershoot(double kp, double ki, double kd,
                                               const PlantModel& plant) const {
    // Simplified overshoot estimation
    double wn = std::sqrt(kp / plant.time_constant);  // Natural frequency
    double zeta = (kd + 1.0 / (2.0 * wn * plant.time_constant)) / 2.0;  // Damping ratio

    if (zeta < 0.1) zeta = 0.1;
    if (zeta > 1.0) return 0.0;  // Overdamped, no overshoot

    // Approximate overshoot formula
    double overshoot = 100.0 * std::exp(-M_PI * zeta / std::sqrt(1 - zeta * zeta));
    return overshoot;
}

double IParameterOptimizer::estimateSettlingTime(double kp, double ki, double kd,
                                                  const PlantModel& plant) const {
    // Simplified settling time estimation (4% criterion)
    double wn = std::sqrt(kp / plant.time_constant);
    double zeta = std::min(1.0, (kd + 1.0 / (2.0 * wn * plant.time_constant)) / 2.0);

    if (zeta < 0.1) zeta = 0.1;

    double settling_time = 4.0 / (zeta * wn);
    return settling_time;
}

//=============================================================================
// ZieglerNicholsOptimizer
//=============================================================================

OptimizationResult ZieglerNicholsOptimizer::optimize(
    const PlantModel& plant,
    const nlohmann::json& current_params,
    const OptimizationConstraints& constraints
) {
    OptimizationResult result;
    result.success = false;
    result.method_used = getName();

    // Ziegler-Nichols ultimate gain method
    // For a first-order system with delay, we can use Cohen-Coon variant

    double ku = 4.0 / (plant.gain * plant.time_constant);  // Ultimate gain
    double tu = plant.time_constant * 4.0;  // Ultimate period

    // Classic Ziegler-Nichols tuning rules
    double kp = 0.6 * ku;
    double ki = 1.2 * ku / tu;
    double kd = 0.075 * ku * tu;

    // Apply safety factor
    kp *= 0.8;
    ki *= 0.8;
    kd *= 0.8;

    nlohmann::json optimized_params;
    optimized_params["kp"] = kp;
    optimized_params["ki"] = ki;
    optimized_params["kd"] = kd;

    if (validateParameters(optimized_params, constraints)) {
        result.parameters = optimized_params;
        result.final_cost = calculateCost(plant, optimized_params, constraints);
        result.success = true;
        result.iterations = 1;
    }

    return result;
}

//=============================================================================
// GeneticAlgorithmOptimizer
//=============================================================================

GeneticAlgorithmOptimizer::GeneticAlgorithmOptimizer()
    : config_{} {
}

GeneticAlgorithmOptimizer::GeneticAlgorithmOptimizer(const GAConfig& config)
    : config_(config) {
}

OptimizationResult GeneticAlgorithmOptimizer::optimize(
    const PlantModel& plant,
    const nlohmann::json& current_params,
    const OptimizationConstraints& constraints
) {
    OptimizationResult result;
    result.success = false;
    result.method_used = getName();
    result.iterations = config_.generations;

    // Initialize parameter bounds
    nlohmann::json bounds = {
        {"kp", {{"min", 0.01}, {"max", 10.0}}},
        {"ki", {{"min", 0.0}, {"max", 5.0}}},
        {"kd", {{"min", 0.0}, {"max", 2.0}}}
    };

    // Use constraint ranges if available
    if (constraints.param_ranges.contains("kp")) {
        bounds["kp"] = constraints.param_ranges["kp"];
    }
    if (constraints.param_ranges.contains("ki")) {
        bounds["ki"] = constraints.param_ranges["ki"];
    }
    if (constraints.param_ranges.contains("kd")) {
        bounds["kd"] = constraints.param_ranges["kd"];
    }

    // Initialize population
    auto population = initializePopulation(config_.population_size, bounds);

    // Evaluate initial population
    for (auto& individual : population) {
        individual.fitness = evaluateFitness(individual, plant, constraints);
    }

    // Sort by fitness (lower cost = better fitness)
    std::sort(population.begin(), population.end(),
        [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });

    // Evolution loop
    for (int gen = 0; gen < config_.generations; ++gen) {
        result.cost_history.push_back(population[0].fitness);

        // Selection
        auto selected = selection(population);

        // Create new generation
        std::vector<Individual> new_population;
        new_population.reserve(config_.population_size);

        // Elitism - keep best individuals
        for (int i = 0; i < config_.elite_count && i < static_cast<int>(population.size()); ++i) {
            new_population.push_back(population[i]);
        }

        // Crossover and mutation
        while (new_population.size() < static_cast<size_t>(config_.population_size)) {
            // Select parents
            int idx1 = std::uniform_int_distribution<int>(0, selected.size() - 1)(std::mt19937(std::random_device()()));
            int idx2 = std::uniform_int_distribution<int>(0, selected.size() - 1)(std::mt19937(std::random_device()()));

            // Crossover
            auto offspring = crossover(selected[idx1], selected[idx2]);

            // Mutate
            if (std::uniform_real_distribution<double>(0, 1)(std::mt19937(std::random_device()())) < config_.mutation_rate) {
                mutate(offspring, bounds);
            }

            // Evaluate
            offspring.fitness = evaluateFitness(offspring, plant, constraints);
            new_population.push_back(offspring);
        }

        population = std::move(new_population);

        // Sort by fitness
        std::sort(population.begin(), population.end(),
            [](const Individual& a, const Individual& b) {
                return a.fitness < b.fitness;
            });
    }

    // Return best solution
    if (!population.empty()) {
        result.parameters = population[0].genes;
        result.final_cost = population[0].fitness;
        result.success = true;
    }

    return result;
}

std::vector<GeneticAlgorithmOptimizer::Individual> GeneticAlgorithmOptimizer::initializePopulation(
    int size, const nlohmann::json& bounds
) {
    std::vector<Individual> population;
    std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < size; ++i) {
        Individual individual;
        individual.genes = nlohmann::json{};

        std::uniform_real_distribution<double> kp_dist(bounds["kp"]["min"].get<double>(),
                                                      bounds["kp"]["max"].get<double>());
        std::uniform_real_distribution<double> ki_dist(bounds["ki"]["min"].get<double>(),
                                                      bounds["ki"]["max"].get<double>());
        std::uniform_real_distribution<double> kd_dist(bounds["kd"]["min"].get<double>(),
                                                      bounds["kd"]["max"].get<double>());

        individual.genes["kp"] = kp_dist(rng);
        individual.genes["ki"] = ki_dist(rng);
        individual.genes["kd"] = kd_dist(rng);

        population.push_back(individual);
    }

    return population;
}

double GeneticAlgorithmOptimizer::evaluateFitness(
    const Individual& individual,
    const PlantModel& plant,
    const OptimizationConstraints& constraints
) {
    return calculateCost(plant, individual.genes, constraints);
}

std::vector<GeneticAlgorithmOptimizer::Individual> GeneticAlgorithmOptimizer::selection(
    const std::vector<Individual>& population
) {
    // Tournament selection
    std::vector<Individual> selected;
    std::mt19937 rng(std::random_device{}());
    const int tournament_size = 3;

    for (size_t i = 0; i < population.size(); ++i) {
        Individual best = population[
            std::uniform_int_distribution<int>(0, population.size() - 1)(rng)
        ];

        for (int j = 1; j < tournament_size; ++j) {
            Individual candidate = population[
                std::uniform_int_distribution<int>(0, population.size() - 1)(rng)
            ];
            if (candidate.fitness < best.fitness) {
                best = candidate;
            }
        }

        selected.push_back(best);
    }

    return selected;
}

GeneticAlgorithmOptimizer::Individual GeneticAlgorithmOptimizer::crossover(
    const Individual& parent1, const Individual& parent2
) {
    Individual offspring;
    std::mt19937 rng(std::random_device{}());

    // Single-point crossover for each gene
    offspring.genes["kp"] = (std::uniform_real_distribution<double>(0, 1)(rng) < 0.5)
        ? parent1.genes["kp"] : parent2.genes["kp"];
    offspring.genes["ki"] = (std::uniform_real_distribution<double>(0, 1)(rng) < 0.5)
        ? parent1.genes["ki"] : parent2.genes["ki"];
    offspring.genes["kd"] = (std::uniform_real_distribution<double>(0, 1)(rng) < 0.5)
        ? parent1.genes["kd"] : parent2.genes["kd"];

    return offspring;
}

void GeneticAlgorithmOptimizer::mutate(Individual& individual, const nlohmann::json& bounds) {
    std::mt19937 rng(std::random_device{}());

    // Gaussian mutation
    std::normal_distribution<double> mutation(0.0, 0.1);

    double kp = individual.genes["kp"].get<double>();
    double ki = individual.genes["ki"].get<double>();
    double kd = individual.genes["kd"].get<double>();

    kp += mutation(rng) * kp;
    ki += mutation(rng) * ki;
    kd += mutation(rng) * kd;

    // Clamp to bounds
    kp = std::max(bounds["kp"]["min"].get<double>(), std::min(bounds["kp"]["max"].get<double>(), kp));
    ki = std::max(bounds["ki"]["min"].get<double>(), std::min(bounds["ki"]["max"].get<double>(), ki));
    kd = std::max(bounds["kd"]["min"].get<double>(), std::min(bounds["kd"]["max"].get<double>(), kd));

    individual.genes["kp"] = kp;
    individual.genes["ki"] = ki;
    individual.genes["kd"] = kd;
}

//=============================================================================
// BayesianOptimizer
//=============================================================================

BayesianOptimizer::BayesianOptimizer()
    : config_{} {
}

BayesianOptimizer::BayesianOptimizer(const BayesConfig& config)
    : config_(config), sample_history_{} {
}

OptimizationResult BayesianOptimizer::optimize(
    const PlantModel& plant,
    const nlohmann::json& current_params,
    const OptimizationConstraints& constraints
) {
    OptimizationResult result;
    result.success = false;
    result.method_used = getName();
    result.iterations = config_.iterations;

    // TODO: Implement full Gaussian Process-based Bayesian optimization
    // For now, use a simplified random search with exploitation

    nlohmann::json best_params = current_params;
    double best_cost = calculateCost(plant, current_params, constraints);

    sample_history_.push_back({current_params, best_cost});

    std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < config_.iterations; ++i) {
        // Generate candidate around best point (exploitation)
        nlohmann::json candidate = best_params;

        std::normal_distribution<double> perturbation(0.0, 0.1);
        candidate["kp"] = std::max(0.01, candidate["kp"].get<double>() * (1.0 + perturbation(rng)));
        candidate["ki"] = std::max(0.0, candidate["ki"].get<double>() * (1.0 + perturbation(rng)));
        candidate["kd"] = std::max(0.0, candidate["kd"].get<double>() * (1.0 + perturbation(rng)));

        if (validateParameters(candidate, constraints)) {
            double cost = calculateCost(plant, candidate, constraints);

            if (cost < best_cost) {
                best_cost = cost;
                best_params = candidate;
            }

            result.cost_history.push_back(cost);
            sample_history_.push_back({candidate, cost});
        }
    }

    result.parameters = best_params;
    result.final_cost = best_cost;
    result.success = true;

    return result;
}

//=============================================================================
// OptimizerRegistry
//=============================================================================

OptimizerRegistry& OptimizerRegistry::instance() {
    static OptimizerRegistry registry;
    return registry;
}

void OptimizerRegistry::registerOptimizer(const std::string& name,
                                         std::shared_ptr<IParameterOptimizer> optimizer) {
    optimizers_[name] = optimizer;
}

std::shared_ptr<IParameterOptimizer> OptimizerRegistry::getOptimizer(const std::string& name) {
    auto it = optimizers_.find(name);
    if (it != optimizers_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> OptimizerRegistry::listOptimizers() const {
    std::vector<std::string> names;
    for (const auto& pair : optimizers_) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace roboclaw::embedded
