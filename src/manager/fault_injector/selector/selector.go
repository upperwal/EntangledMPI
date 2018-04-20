package selector

import (
	"bufio"
	"errors"
	"time"
	"os"
	"strings"
	"strconv"

	"manager/rng"
)

// Supports only two process in a job.
type Job struct {
	CR [2]int
}

func initReplicationMap(replicationMapFile string) map[int]*Job {
	replicationMap := make(map[int]*Job)

	file, err := os.Open(replicationMapFile)
	if err != nil {
		panic(err)
	}

	defer file.Close()

	reader := bufio.NewReader(file)

	line, _ := reader.ReadString('\n')

	lineSplit := strings.Split(strings.Replace(line, "\n", "", -1), "\t")

	numJobs, _ := strconv.Atoi(lineSplit[1])
	for i := 0; i < numJobs; i++ {
		line, _ := reader.ReadString('\n')
		lineSplit := strings.Split(strings.Replace(line, "\n", "", -1), "\t")

		jobID, _ := strconv.Atoi(lineSplit[1])

		replicationMap[jobID] = &Job{}
		replicationMap[jobID].CR[0] = -1
		replicationMap[jobID].CR[1] = -1
		for j := 3; j < len(lineSplit); j++ {
			rank, _ := strconv.Atoi(lineSplit[j])
			replicationMap[jobID].CR[j - 3] = rank
		}
	}

	return replicationMap
}

type QueuedProcesses struct {
	Rank int
	WaitInterval time.Duration
}

type SelectorOpts struct {
	// Replica, Compute or Both
	ImageToKill int

	// Only kills replicated nodes (compute or replica) and does not let the application crash.
	// Default True
	KillEvenIfNotReplicated bool
}

type Selector struct {
	Dist rng.Generator
	Intervals int
	RepMap map[int]*Job
	RankQueued map[int]bool
	Opts *SelectorOpts
}

const (
	KILL_COMPUTE int = iota
	KILL_REPLICA
	KILL_ANY
)



var uniformRandomForJobs *rng.UniformGenerator

func NewSelector(replicationMapFile string, dist rng.Generator, duration, interval time.Duration, opt ...*SelectorOpts) (Selector, error) {
	inter := (duration / interval * time.Second).Seconds()
	repMap := initReplicationMap(replicationMapFile)
	rankQueued := make(map[int]bool)

	uniformRandomForJobs = rng.NewUniformGenerator(time.Now().UnixNano())

	if len(opt) == 0 {
		return Selector{dist, int(inter), repMap, rankQueued, nil}, nil
	} else {
		return Selector{dist, int(inter), repMap, rankQueued, opt[0]}, nil
	} 
	
}

func (s Selector) getNew() (*QueuedProcesses, error)  {

	cnt := 0
	for {
		randomJob := int(uniformRandomForJobs.Int32n(int32(len(s.RepMap))))

		killEvenIfNotReplicated := false

		// Kill Compute
		randomRankIndex := 0

		if s.Opts != nil {

			if s.Opts.ImageToKill == KILL_ANY {
				randomRankIndex = int(uniformRandomForJobs.Int32n(2))
			} else if s.Opts.ImageToKill == KILL_REPLICA {
				randomRankIndex = 1
			}

			killEvenIfNotReplicated = s.Opts.KillEvenIfNotReplicated

		}
		
		
		randomRank := s.RepMap[randomJob].CR[randomRankIndex]

		if randomRank < 0 {
			continue
		}

		if !killEvenIfNotReplicated {
			if s.RepMap[randomJob].CR[1 - randomRankIndex] == -1 || s.RankQueued[s.RepMap[randomJob].CR[1 - randomRankIndex]] {
				continue
			}
		}

		if s.RankQueued[randomRank] == true {
			if cnt == 5 {
				return nil, errors.New("This Rank already queued to be dead")
			}
			cnt++
			continue
		}
		
		interval, _ := s.Dist.Next()
		s.RankQueued[randomRank] = true

		intervalDur := time.Duration(1 / interval)

		return &QueuedProcesses{randomRank, time.Second * intervalDur}, nil
	}
	
}

func (s Selector) Next() (*QueuedProcesses, error) {
	return s.getNew()
}

func (s Selector) NextN() ([]*QueuedProcesses, error) {
	queuedArray := make([]*QueuedProcesses, 0)

	for i := 0; i < s.Intervals; i++ {
		q, err := s.Next()
		if err != nil {
			continue
		}
		queuedArray = append(queuedArray, q)
	}

	return queuedArray, nil
}


