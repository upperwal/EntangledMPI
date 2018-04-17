package rng

import (
	"errors"
	"fmt"
	"math"
)

// PoissonGenerator is a random number generator for possion distribution.
// The zero value is invalid, use NewPoissonGenerator to create a generator
type PoissonGenerator struct {
	lambda float64
	uniform *UniformGenerator
}

// NewPoissonGenerator returns a possion-distribution generator
// it is recommended using time.Now().UnixNano() as the seed, for example:
// prng := rng.NewPoissonGenerator(time.Now().UnixNano())
func NewPoissonGenerator(seed int64, opt ...float64) *PoissonGenerator {
	urng := NewUniformGenerator(seed)

	if len(opt) == 1 {
		return &PoissonGenerator{opt[0], urng}
	}
	return &PoissonGenerator{-99999, urng}
}

func (prng PoissonGenerator) Next() (float64,error) {
	if prng.lambda == -99999 {
		return -1, errors.New("Lambda not set")
	}

	return float64(prng.Poisson(prng.lambda)), nil
}

// Poisson returns a random number of possion distribution
func (prng PoissonGenerator) Poisson(lambda float64) int64 {
	if !(lambda > 0.0) {
		panic(fmt.Sprintf("Invalid lambda: %.2f", lambda))
	}
	return prng.poisson(lambda)
}

func (prng PoissonGenerator) poisson(lambda float64) int64 {
	// algorithm given by Knuth
	L := math.Pow(math.E, -lambda)
	var k int64 = 0
	var p float64 = 1.0

	for p > L {
		k++
		p *= prng.uniform.Float64()
	}
	return (k - 1)
}
