package rng

type Generator interface {
	Next() (float64,error)
}