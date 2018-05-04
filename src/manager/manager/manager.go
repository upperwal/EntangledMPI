package main

import (
	"fmt"
	"flag"
	"time"
	"os"
	"strconv"
	"sort"

	"manager/rng"
)

func main() {

	jobs := flag.Int("j", 0, "No of jobs")
	ranks := flag.Int("r", 0, "Total ranks spawnned")
	choose := flag.Int("c", 0, "No of ranks to choose to replace in each iteration")
	timeR := flag.Int("t", 15, "Time between replication map updates")
	flag.Parse()

	if *jobs == 0 || *ranks == 0 {
		fmt.Println("Please provide no of jobs and ranks ex: 'manager -j 10 -r 14'")
		os.Exit(1)
	}

	if *choose == 0 {
		*choose = *ranks - *jobs
	}

	fmt.Println("\nProcess Manager (simulation) started...")
	fmt.Println("|-------------------------------|")
	fmt.Println("|  No. of Jobs \t| No of Ranks  \t|")
	fmt.Printf("|  %d    \t| %d     \t|\n", *jobs, *ranks)
	fmt.Println("|-------------------------------|\n")

	_jm := InitJobMap(*jobs, *ranks)
	//jm := _jm.JMap
	
	for i := 0; i < 10; i++ {
		c := _jm.Choose(*choose)
		_jm.Assign(c)

		fmt.Printf("--> %d of %d updates\n", i+1, 10)

		/*for _jid, _j := range jm {
			fmt.Println(_jid, _j.GetNumberOfRanks())
		}*/

		_jm.WriteToFile("replication.map")

		_jm.ResetModified()

		dur := time.Duration(*timeR) * time.Second
		time.Sleep(dur)
	}
	
}

type Job struct {
	JobID int
	RankList []int
	Modified int
}

func NewJob(jobID int) *Job {
	return &Job{jobID, make([]int, 0), 1}
}

func (j *Job) InsertRank(rank int) {
	j.RankList = append(j.RankList, rank)
}

func (j Job) GetRank(i int) int {
	if len(j.RankList) < i+1 {
		return -1
	}

	return j.RankList[i]
}

func (j Job) GetNumberOfRanks() int {
	return len(j.RankList)
}

func (j *Job) RemoveI(i int) {
	j.RankList = append(j.RankList[:i], j.RankList[i+1:]...)
} 

func (j Job) String() string {
	return string(j.JobID)
}

type JobMap struct {
	JMap map[int]*Job
	RepFile *os.File
}

func InitJobMap(j_no, r_no int) *JobMap {

	jm := &JobMap{make(map[int]*Job, j_no), nil}

	for i := 0; i < j_no; i++ {
		jm.JMap[i] = NewJob(i)
	}

	for i := 0; i < r_no; i++ {
		jm.JMap[i % j_no].InsertRank(i)
	}

	return jm
}

func (jm *JobMap) OpenRepFile(fileName string) {
	f, _ := os.Create(fileName)

	jm.RepFile = f
}

func (jm *JobMap) CloseRepFile() {
	jm.RepFile.Close()
	jm.RepFile = nil
}

func (jm JobMap) GetJob(j int) *Job {
	return jm.JMap[j]
}

func (jm JobMap) CountTotalRanks() int {
	c := 0
	for _, j := range jm.JMap {
		c += len(j.RankList)
	}
	return c
}

func (jm *JobMap) ResetModified() {
	for _, j := range jm.JMap {
		j.Modified = 0
	}
}

func (jm *JobMap) Choose(n int) []int {
	c := make([]int, 0)

	uniformRandomForJobs := rng.NewUniformGenerator(time.Now().UnixNano())

	for i := 0; i < len(jm.JMap); i++ {
		if len(c) >= n {
			return c
		}

		randomJob := int(uniformRandomForJobs.Int32n(int32(len(jm.JMap))))

		if jm.JMap[randomJob].GetNumberOfRanks() == 1 {
			continue
		}

		randomRankIndex := int(uniformRandomForJobs.Int32n(2))

		rank := jm.JMap[randomJob].GetRank(randomRankIndex)
		c = append(c, rank)
		jm.JMap[randomJob].RemoveI(randomRankIndex)
	}

	return c
}

func (jm *JobMap) Assign(newList []int) {
	uniformRandomForJobs := rng.NewUniformGenerator(time.Now().UnixNano())
	allocationCounter := 0

	for i := 0; i < len(jm.JMap) * 2; i++ {
		if allocationCounter >= len(newList) {
			break
		}

		randomJob := int(uniformRandomForJobs.Int32n(int32(len(jm.JMap))))

		if jm.JMap[randomJob].GetNumberOfRanks() == 2 {
			continue
		}

		jm.JMap[randomJob].InsertRank(newList[allocationCounter])
		jm.JMap[randomJob].Modified = 1
		allocationCounter++
	}

	newList = newList[:0]
}

func (jm JobMap) WriteToFile(name string) {
	jm.OpenRepFile(name)
	jm.RepFile.WriteString(strconv.Itoa(jm.CountTotalRanks()))
	jm.RepFile.WriteString("\t")
	jm.RepFile.WriteString(strconv.Itoa(len(jm.JMap)))
	jm.RepFile.WriteString("\n")

	var keys []int
    for k := range jm.JMap {
        keys = append(keys, k)
    }

	sort.Ints(keys)
	for _, k := range keys {
		j := jm.JMap[k]
		jm.RepFile.WriteString(strconv.Itoa( j.Modified ))
		jm.RepFile.WriteString("\t")
		jm.RepFile.WriteString(strconv.Itoa( k ))
		jm.RepFile.WriteString("\t")
		jm.RepFile.WriteString(strconv.Itoa( len(j.RankList) ))

		for _, v := range j.RankList {
			jm.RepFile.WriteString("\t")
			jm.RepFile.WriteString(strconv.Itoa( v ))
		
		}
		jm.RepFile.WriteString("\n")
	} 

	jm.RepFile.WriteString("[first line: <TOTAL_CORES><TAB><NO_OF_JOBS>]\n")
	jm.RepFile.WriteString("[followed by (in each line): <UPDATE_BIT><TAB><JOB_ID><TAB><NO_OF_WORKERS><TAB><ORIGINAL_RANK_1><TAB><ORIGINAL_RANK_2><TAB>...]\n")
	jm.RepFile.WriteString("<JOB_ID> starts with 0\n")
	jm.RepFile.WriteString("<ORIGINAL_RANK_*> starts with 0")
	
	jm.CloseRepFile()


}
