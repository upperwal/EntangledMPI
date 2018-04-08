package main

import (
	"bytes"
	"fmt"
	"flag"
	"math/rand"
	"os/exec"
	"strings"
)

func main() {
	repFile := flag.String("mp", "", "Replication Map file")
	programName := flag.String("prog", "", "Rep MPI Program name")
	flag.Parse()

	if *repFile == "" {
		fmt.Println("Enter a replication file path")
		panic("No Rep File")
	}

	if *programName == "" {
		fmt.Println("Enter a MPI program name to kill")
		panic("No MPI Program")
	}

	fmt.Println("Replication File: ", *repFile)
	fmt.Println("MPI Program: ", *programName)

	pidofCMD := exec.Command("pidof", *programName)
	var out bytes.Buffer
	pidofCMD.Stdout = &out
	if err := pidofCMD.Run(); err != nil {
		panic(err)
	} else {
		pid := strings.Split(strings.Replace(out.String(), "\n", "", -1), " ")
		
		for i, p := range pid {
			proc := rand.Intn(len(pid))
			if i == proc {
				err := exec.Command("kill", "-9", p).Run()
				fmt.Println("Killed ", p, i, err == nil)
			}
			fmt.Println("PID: ", p, proc)
		}
	}
}