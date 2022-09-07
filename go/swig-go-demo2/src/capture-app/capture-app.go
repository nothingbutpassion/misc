package main

import "fmt"
import "capture"

func main() {
	c := capture.NewCapture()

	if c.Open(0) != true {
		fmt.Println("can't open camera")
	}
	
	c.CreateWindow("edges");

	for {
		if c.Read() != true {
			fmt.Println("can't read frame")		
		} else {
			c.Edge()
			c.Show("edges")
			if c.WaitKey(30) != -1 {
				break;		
			}
		}
	}

	c.DestroyWindow("edges")

	capture.DeleteCapture(c)
}
