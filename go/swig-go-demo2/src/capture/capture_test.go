package capture

import "testing"

func TestCapture(t *testing.T) {

	c := NewCapture()

	if c.Open(0) != true {
		t.Errorf("can't open camera")
	}
	
	c.CreateWindow("edges");

	for {
		if c.Read() != true {
			t.Errorf("can't read frame")		
		} else {
			c.Edge()
			c.Show("edges")
			if c.WaitKey(30) != -1 {
				break;		
			}
		}
	}

	c.DestroyWindow("edges")

	DeleteCapture(c)
}
