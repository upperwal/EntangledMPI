// Package rng implements a series of pseudo-random number generator
// based on a variety of common probability distributions
package rng

import (
	"errors"
	"math"
)

// GaussianGenerator is a random number generator for gaussian distribution.
// The zero value is invalid, use NewGaussianGenerator to create a generator
type GaussianGenerator struct {
	mean, stddev float64
	uniform *UniformGenerator
}

// NewGaussianGenerator returns a gaussian-distribution generator
// it is recommended using time.Now().UnixNano() as the seed, for example:
// crng := rng.NewGaussianGenerator(time.Now().UnixNano())
func NewGaussianGenerator(seed int64, opt ...float64) *GaussianGenerator {
	urng := NewUniformGenerator(seed)

	if len(opt) == 2 {
		return &GaussianGenerator{opt[0], opt[1], urng}
	}
	return &GaussianGenerator{-99999, -99999, urng}
}

func (grng GaussianGenerator) Next() (float64,error) {
	if grng.mean == -99999 || grng.stddev == -99999 {
		return -1, errors.New("Mean and Variance not set")
	}
	return grng.mean + grng.stddev*grng.gaussian(), nil
}

// StdGaussian returns a random number of standard gaussian distribution
func (grng GaussianGenerator) StdGaussian() float64 {
	return grng.gaussian()
}

// Gaussian returns a random number of gaussian distribution Gauss(mean, stddev^2)
func (grng GaussianGenerator) Gaussian(mean, stddev float64) float64 {
	return mean + stddev*grng.gaussian()
}

func (grng GaussianGenerator) gaussian() float64 {
	// Box-Muller Transform
	var r, x, y float64
	for r >= 1 || r == 0 {
		x = grng.uniform.Float64Range(-1.0, 1.0)
		y = grng.uniform.Float64Range(-1.0, 1.0)
		r = x*x + y*y
	}
	return x * math.Sqrt(-2*math.Log(r)/r)
}
