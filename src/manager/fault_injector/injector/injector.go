package injector

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"unsafe"
	"os/exec"
	"time"

	"manager/fault_injector/selector"
)

type NetworkMap struct {
	Rank uint32
	PID uint32
	Len uint32
	Hostname [100]byte
}

func initNetworkMap(networkMapFile string) map[uint32]*NetworkMap {
	rank2Host := make(map[uint32]*NetworkMap)

	file, err := os.Open(networkMapFile)
	if err != nil {
		panic(err)
	}

	defer file.Close()

	data := make([]byte, unsafe.Sizeof(NetworkMap{}))
	
	for {
		_, err = file.Read(data)
		if err != nil {
			break
		}

		netMap := new(NetworkMap)

		buffer := bytes.NewBuffer(data)
		err = binary.Read(buffer, binary.LittleEndian, netMap)
		if err != nil {
			fmt.Println("Err")
		}

		rank2Host[netMap.Rank] = netMap

		fmt.Printf("Parse data: %d %d %s\n", netMap.Rank, netMap.PID, netMap.Hostname[:netMap.Len])
	}
	

	
	/*file, _ := os.Open(replicationMapFile)

	reader := bufio.NewReader(file)

	line, _ := reader.ReadString('\n')

	dat := strings.Split(line, "\t")
	fmt.Println(dat[0], dat[1])
*/
	return rank2Host
}

type Injector struct {
	NetworkMap map[uint32]*NetworkMap
}

func NewInjector(networkMapFile string) (Injector, error) {
	netMap := initNetworkMap(networkMapFile)

	return Injector{netMap}, nil
}

func (i Injector) Start(list []*selector.QueuedProcesses) {
	fmt.Println("Starting Fault Injection...")

	for _, p := range list {
		time.Sleep(p.WaitInterval)

		netMap := i.lookup(p.Rank)

		i.kill(netMap)
	}
}

func (i Injector) lookup(rank int) *NetworkMap {
	return i.NetworkMap[uint32(rank)]
}

func (i Injector) kill(p *NetworkMap) {
	rank := p.Rank
	pid := p.PID
	host := string(p.Hostname[:p.Len])

	fmt.Println("Killing: ", rank, pid, host)
	
	killCommand := fmt.Sprintf("'kill -9 %d'", pid)
	pidofCMD := exec.Command("ssh", "-t", host, killCommand)
	var out bytes.Buffer
	pidofCMD.Stdout = &out
	if err := pidofCMD.Run(); err != nil {
		panic(err)
	} else {
		fmt.Println("OUTPUT: ", out)
		
		/*for i, p := range pid {
			proc := rand.Intn(len(pid))
			if i == proc {
				err := exec.Command("ssh -t node1 'kill -9 '", "-9", p).Run()
				fmt.Println("Killed ", p, i, err == nil)
			}
			fmt.Println("PID: ", p, proc)
		}*/
	}
}
