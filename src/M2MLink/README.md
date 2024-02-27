The M2MLink classes (machine-to-machine link) implement a binary protocol with optional CBOR encoding for payloads. Each instance of M2MLink represents a persistent dialog with another machine via some unspecified transport.

Underlying transports can be connectionless, or point-to-point. MTU, encoding arrangements, and type support is negotiated by the class, and a unform API presented to the programs at either end of the link.


## The protocol itself

### Design goals
  * The protocol and packets should be as light as allowable to achieve the other design goals. Ideally, this will allow transparent operation with most isochronous and bulk transfer duties, as permitted by the underlying transports.
  * Vagaries of version control should be handled by the class. Connections advertising an unsupported version should be dropped.
  * The class should bridge the gap between type transcription deltas between the two counterparties. That is: C3PTypes supported on one side of the link should be relatable to the other.
  * The link should be capable of providing Identies of counterparties. Both from the voluntary reporting of the counterparty itself, and/or from the transport (if supported). The class should identify the source of the identity information it reports.
  * Both sides of the link should be capable of coordinated self-termination for the sake of relinquishing the transport for some other protocol's usage. This is not a strict requirement, but programs that might benefit from the capability are encouraged to support it.


### Framing

All multibyte values are stored "little-endian".
Checksum = 0x55 (Preload) + Every other byte after it. Then truncate to uint8.

|<-------------------HEADER------------------------>|
|--------------------------------------------------|  /---------------\
| Total Length | Checksum | Unique ID | Message ID |  | Optional data |
|--------------------------------------------------|  \---------------/
      3             1           2           2            <---- Bytes


### Sync

Minimum packet size is the sync packet (4 bytes). Sync packet is
always the same (for a given CHECKSUM_PRELOAD):
0x04 0x00 0x00 0x55
|----- Checksum preload

Sync packets are the way in which we establish initial offset for a dialog over a stream.
We need to sync initially on connection or if something goes sideways in the course
of a normal dialog.

Receiving an unsolicited sync packet should be an indication that the counterparty either
can't understand us, or the transport dropped a message and they timed it out. When we see
a sync packet on the wire, we should allow the currently-active transaction to complete and
then send back a sync stream of our own. When the counterparty notices this, communication
can resume.


## Session life-cycle

### Sequence numbers



### Capability discovery

#### MTU negotiation

#### Encoding and type support negotiation

#### Transports

#### Protocols

#### Cryptography

#### Hardware capabilities


### Configuration

### Identity management


### Sending

#### To a specific identity

#### Broadcasting

#### Selecting a protocol and/or transport

#### Console


### Cached messages

### Time and Date

### Location

### Display


----------------------------
# Usage

## API presnted to the software (writing against M2MLink)


## API between firmware instances (getting two sides of a link to talk)
