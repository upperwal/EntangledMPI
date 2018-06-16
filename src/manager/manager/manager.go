package main

import (
	"fmt"
	"flag"
	"time"
	"os"
	"strconv"
	"sort"
	"bytes"
	"encoding/binary"
	"unsafe"
	

	"manager/rng"
)

func main() {

	jobs := flag.Int("j", 0, "No of jobs")
	ranks := flag.Int("r", 0, "Total ranks spawnned")
	choose := flag.Int("c", 0, "No of ranks to choose to replace in each iteration")
	timeR := flag.Int("t", 15, "Time between replication map updates")
	nosUpdates := flag.Int("i", 4, "Nos of update iteration")
	repidxFilename := flag.String("rep", "rep_stack.info", "MPI program will save rep index and stack address in this file")
	flag.Parse()

	if *jobs == 0 || *ranks == 0 {
		fmt.Println("Please provide no of jobs and ranks ex: 'manager -j 10 -r 14'")
		os.Exit(1)
	}

	if *jobs > *ranks {
		fmt.Println("Error: jobs <= ranks")
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

	// Create checkpoint directory
	os.Mkdir("./ckpt", 0775)
	//jm := _jm.JMap
	
	for i := 0; i < *nosUpdates; i++ {
		c := _jm.Choose(*choose)
		_jm.Assign(c)

		fmt.Printf("--> %d of %d updates\n", i+1, *nosUpdates)

		/*for _jid, _j := range jm {
			fmt.Println(_jid, _j.GetNumberOfRanks())
		}*/
		_jm.String()
		_jm.WriteToFile("replication.map")

		_jm.ResetModified()

		dur := time.Duration(*timeR) * time.Second
		
		if i < *nosUpdates - 1 {
			time.Sleep(dur)
		}

		//for _jm.InitStackData(*repidxFilename, i != 0) {}
		_jm.InitStackData(*repidxFilename, i != 0)
	}
	
}

type Rank struct {
	Rank int
	PrevJob int
	StackAdd uint64
}

type Job struct {
	JobID int
	RankList []Rank
	Modified int
}

func NewJob(jobID int) *Job {
	return &Job{jobID, make([]Rank, 0), 1}
}

func (j *Job) InsertRank(rank Rank) {
	j.RankList = append(j.RankList, rank)
}

func (j Job) GetRank(i int) Rank {
	if len(j.RankList) < i+1 {
		return Rank{}
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
	RepIDx int
}

func InitJobMap(j_no, r_no int) *JobMap {

	jm := &JobMap{make(map[int]*Job, j_no), nil, 0}

	for i := 0; i < j_no; i++ {
		jm.JMap[i] = NewJob(i)
	}

	for i := 0; i < r_no; i++ {
		jm.JMap[i % j_no].InsertRank(Rank{i, -1, 0})
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

func (jm *JobMap) Choose(n int) []Rank {
	c := make([]Rank, 0)

	uniformRandomForJobs := rng.NewUniformGenerator(time.Now().UnixNano())

	for i := 0; i < len(jm.JMap) / 2; i++ {
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
		jm.JMap[randomJob].Modified = 1
	}
	fmt.Println(c)
	return c
}

func (jm *JobMap) Assign(newList []Rank) {
	uniformRandomForJobs := rng.NewUniformGenerator(time.Now().UnixNano())
	allocationCounter := 0

	for len(newList) > allocationCounter {
		if allocationCounter >= len(newList) {
			break
		}

		randomJob := int(uniformRandomForJobs.Int32n(int32(len(jm.JMap))))

		if jm.JMap[randomJob].GetNumberOfRanks() == 2 {
			continue
		}

		if newList[allocationCounter].PrevJob != randomJob && jm.JMap[randomJob].RankList[0].StackAdd == newList[allocationCounter].StackAdd {
			jm.JMap[randomJob].InsertRank(newList[allocationCounter])
			jm.JMap[randomJob].Modified = 1
			allocationCounter++
		}
	}

	newList = newList[:0]
}

func (jm JobMap) String() {
	fmt.Printf(strconv.Itoa(jm.CountTotalRanks()))
	fmt.Printf("\t")
	fmt.Printf(strconv.Itoa(len(jm.JMap)))
	fmt.Printf("\n")

	var keys []int
    for k := range jm.JMap {
        keys = append(keys, k)
    }

	sort.Ints(keys)
	for _, k := range keys {
		j := jm.JMap[k]
		fmt.Printf(strconv.Itoa( j.Modified ))
		fmt.Printf("\t")
		fmt.Printf(strconv.Itoa( k ))
		fmt.Printf("\t")
		fmt.Printf(strconv.Itoa( len(j.RankList) ))

		for _, v := range j.RankList {
			fmt.Printf("\t")
			fmt.Printf(strconv.Itoa( v.Rank ))
		
		}
		fmt.Printf("\n")
	} 
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
			jm.RepFile.WriteString(strconv.Itoa( v.Rank ))
		
		}
		jm.RepFile.WriteString("\n")
	} 

	jm.RepFile.WriteString("[first line: <TOTAL_CORES><TAB><NO_OF_JOBS>]\n")
	jm.RepFile.WriteString("[followed by (in each line): <UPDATE_BIT><TAB><JOB_ID><TAB><NO_OF_WORKERS><TAB><ORIGINAL_RANK_1><TAB><ORIGINAL_RANK_2><TAB>...]\n")
	jm.RepFile.WriteString("<JOB_ID> starts with 0\n")
	jm.RepFile.WriteString("<ORIGINAL_RANK_*> starts with 0")
	
	jm.CloseRepFile()


}

type StackAddrRepIdx struct {
	RepIDx int32
	Rank int32
	StackAdd uint64
}

type Header struct {
	UserDataMaxSize uint32
	HeaderOffset uint32
	UserDataSize uint32
	_ [5]byte
	Starcraft2 [22]byte
}

func (jm *JobMap) InitStackData(fileName string, onlyRepIDx bool) bool {
	file, err := os.Open(fileName)
	if err != nil {
		panic(err)
	}

	defer file.Close()

	mappings := make(map[int32]uint64)

	data := make([]byte, unsafe.Sizeof(StackAddrRepIdx{}))

	stackInfo := StackAddrRepIdx{}
	for {
		_, err = file.Read(data)
		if err != nil {
			break
		}

		buffer := bytes.NewBuffer(data)
		err = binary.Read(buffer, binary.LittleEndian, &stackInfo)
		if err != nil {
			panic(err)
		}

		if onlyRepIDx {
			if stackInfo.RepIDx != int32(jm.RepIDx) {
				jm.RepIDx = int(stackInfo.RepIDx)
				return false
			}
			return true
		}

		mappings[stackInfo.Rank] = uint64(stackInfo.StackAdd)

		fmt.Printf("Parse data: Rank: %d | StackAdd: %#x\n", stackInfo.Rank, stackInfo.StackAdd)
	}

	jm.RepIDx = int(stackInfo.RepIDx)

	for _, j := range jm.JMap {
		for r, _ := range j.RankList {
			j.RankList[r].StackAdd = mappings[int32(j.RankList[r].Rank)]
		}
	}

	return true
}
