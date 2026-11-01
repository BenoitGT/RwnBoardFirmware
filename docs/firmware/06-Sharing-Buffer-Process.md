---
tags: [sdd, dcc-control-app, contrat]
banner: "[[microchip_banner.png]]"
banner_y: 70
---
# Sharing buffer process

 ```mermaid
 block-beta
 columns 1
	block:buffer
		columns 4
		pk1["Packet 1"]
		pk2["Packet 2"]
		block:pk3["Packet 3"]
			columns 6
			b0
			b1
			b2
			b3
			b4
			b5
		end
		pk4["Packet 4"]
	end
	space
	block:pointer
		columns 4
		head
		commited
		tail
	end
	head --> pk1
	commited --> pk2
	tail --> b2
	b2 --> pk2
	
	classDef read fill:#c0392b,stroke:#fde2e2
    classDef commited fill:#ffb8b8,stroke:#dfb8b8
    classDef writing fill:#ffd8d8,stroke:#dfe8e8
    class pk1,head read 
    class pk2,commited commited 
    class pk3,b0,b1,tail writing
    
 ```


## Fill-Up

- `rwnBus_taskInterrupt()` write byte at `tail` and increment `tail` . This not impact the `commited`pointer position.
- When `rwnBus_Task`  validate the current frame (size + XOR), `commited`  move to `tail` . the end of current written packet. data became available and `RWNBUS_FRAME_READY() == 1`.
- if frame is invalid, `Rollback` set `tail` to `commited`. all Byte of the packet became back writable.
- at the end of the buffer if `head`!=0 ,the `tail`is set to the beginning `tail=0`
## Reading and free
- When `RWNBUS_FRAME_READY() == 1` , `dccDecoder_task()` process the instruction. then set `TRACK_TRM=1` to start transmission on DCC.
- When trackPower_taskInterrupt() ends transmitted packet (`TRACK_TRM` is 0 ) .`RWNBUS_RELEASE()` is called and `head`  is set to the beginning of next packet.

## Empty or Full
- The buffer is empty when `head`is `commited`
- The buffer is full when `tail + 1` == `head`