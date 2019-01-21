/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using namespace std;
using std::string;
using std::vector;

default_random_engine gen;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
    // set number of particles
  num_particles = 50;
  
  // GPS uncertainties as normal distribution
  normal_distribution<double> dist_x(x, std[0]);
  normal_distribution<double> dist_y(y, std[1]);
  normal_distribution<double> dist_theta(theta, std[2]);

  // size particles and weights to specified number of particles
  particles.resize(num_particles);
  weights.resize(num_particles);

  // initialize particles to first GPS position with uncertainty
  for (int i=0; i < num_particles; i++){
    particles[i].id = i;
    particles[i].x = dist_x(gen);
    particles[i].y = dist_y(gen);
    particles[i].theta = dist_theta(gen);
    particles[i].weight = 1.0;
  }

  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */

  // random Gaussian sensor noise
  normal_distribution<double> dist_x(0, std_pos[0]);
  normal_distribution<double> dist_y(0, std_pos[1]);
  normal_distribution<double> dist_theta(0, std_pos[2]);

  // predict particles x, y, and theta; then, add random Gaussian noise
  for (int i=0; i < num_particles; i++){
    // avoid division by zero
    if (fabs(yaw_rate) < 0.0001){
      particles[i].x += velocity * delta_t * cos(particles[i].theta);
      particles[i].y += velocity * delta_t * sin(particles[i].theta);
    }
    else{  
      particles[i].x += (velocity/yaw_rate) * (sin(particles[i].theta + yaw_rate*delta_t) - sin(particles[i].theta));
      particles[i].y += (velocity/yaw_rate) * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate*delta_t));
      particles[i].theta += yaw_rate * delta_t;
    }

    // add random Gaussian noise 
    particles[i].x += dist_x(gen);
    particles[i].y += dist_y(gen);
    particles[i].theta += dist_theta(gen);
  }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */

  // get standard deviations and their squares for calculations later
  double std_x = std_landmark[0];
  double std_y = std_landmark[1];
  double std_x2 = std_x * std_x;
  double std_y2 = std_y * std_y;
  double sum_weights = 0.0;

  // calculate normalization term
  double gauss_norm = 1 / (2 * M_PI * std_x * std_y);

  // Transform observations from vehicle coordinates to map coordinates 
  for (int i=0; i < num_particles; i++){
    // get current particle values
    double x_part = particles[i].x;
    double y_part = particles[i].y;
    double theta_part = particles[i].theta;

    // create object to store all landmarks within sensor range
    Map predicted;
    
    // loop through landmarks to find those within sensor range
    for (unsigned int k=0; k < map_landmarks.landmark_list.size(); k++){
      
      // create single landmark instance to store current landmark
      Map::single_landmark_s single_landmark_temp = map_landmarks.landmark_list[k];
      
      // calculate dist from landmark to particle
      double distance = dist(x_part, y_part, single_landmark_temp.x_f, single_landmark_temp.y_f);
      
      // store landmarks within sensor range      
      if (distance <= sensor_range){
        predicted.landmark_list.push_back(single_landmark_temp);
      }

    }
    
    // initialize weight vars
    weights[i] = 1.0;

    // transform observations to map coordinates, find closest landmark to each observation and particle weights 
    for (unsigned int j=0; j < observations.size(); j++){
      double x_om = x_part + (cos(theta_part) * observations[j].x) - (sin(theta_part) * observations[j].y);
      double y_om = y_part + (sin(theta_part) * observations[j].x) + (cos(theta_part) * observations[j].y);

      // var to hold closest distance
      double closest_dist = 1000; // set to large arbitrary value

      // single landmark map instance to store closest landmark 
      Map::single_landmark_s closest_landmark;
      
      // find closest landmark to current observation
      for (unsigned int k=0; k < predicted.landmark_list.size(); k++){
      
        // get x & y coordinates of predicted landmarks
        float x_pred = predicted.landmark_list[k].x_f;
        float y_pred = predicted.landmark_list[k].y_f;

        // calculate distance between observation and landmark
        double distance = dist(x_om, y_om, x_pred, y_pred);

        // find closest landmark
        if (distance < closest_dist){
          closest_dist = distance;
          // store current landmark
          closest_landmark.x_f = x_pred;
          closest_landmark.y_f = y_pred;
          closest_landmark.id_i = predicted.landmark_list[k].id_i;          
        }

      }

      // calculate weight of particle
      double delta_x2 = pow(x_om - closest_landmark.x_f, 2);
      double delta_y2 = pow(y_om - closest_landmark.y_f, 2);
      double exponent = (delta_x2 / (2*std_x2)) + (delta_y2 / (2*std_y2));
      weights[i] *= gauss_norm * exp(-exponent); 
      
    }

    // get sum for normalization calculation
    sum_weights += weights[i]; 
  }

  // normalize weights between [0,1]
  for (int i=0; i < num_particles; i++){
    particles[i].weight = weights[i] / sum_weights;
    weights[i] = particles[i].weight;
  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  // vector to store re-sampled particles
  std::vector<Particle> resampled_particles;
  resampled_particles.resize(num_particles);  //same size as "particles[]"

  // create distributions for beta and index
  uniform_real_distribution<double> dist_beta(0.0, 1.0);
  discrete_distribution<> dist_index(weights.begin(), weights.end());
  int index = dist_index(gen);

  double beta = 0.0;  // initialize beta to zero

  // re-sample particles
  for (int i=0; i < num_particles; i++){
    beta += dist_beta(gen) * 2.0;
    while (weights[index] < beta){
      beta -= weights[index];
      index = (index + 1) % num_particles;
    }

    // store re-sampled particle
    resampled_particles[i] = particles[index];
  }

  // store ALL re-sampled particles
  particles = resampled_particles;
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}