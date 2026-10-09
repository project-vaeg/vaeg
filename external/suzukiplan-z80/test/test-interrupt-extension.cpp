#include "../z80.hpp"

#include <cstring>
#include <iostream>
#include <vector>

static const unsigned char kIFF1 = 0x01;
static const unsigned char kHalt = 0x80;
static const unsigned short kPC = 0x1000;
static const unsigned short kSP = 0xF000;

struct Event {
    char type;
    unsigned short address;
    unsigned char value;

    Event(char eventType, unsigned short eventAddress, unsigned char eventValue)
        : type(eventType), address(eventAddress), value(eventValue)
    {
    }
};

struct Harness {
    unsigned char memory[65536];
    std::vector<Event> events;
    Z80* cpu;
    unsigned char acknowledgeValue;
    int acknowledgeCount;
    int clockTotal;
    bool acknowledgeFromAccumulator;

    Harness()
        : cpu(nullptr), acknowledgeValue(0), acknowledgeCount(0), clockTotal(0), acknowledgeFromAccumulator(false)
    {
        std::memset(memory, 0, sizeof(memory));
    }

    static unsigned char read(void* opaque, unsigned short address)
    {
        Harness* harness = static_cast<Harness*>(opaque);
        unsigned char value = harness->memory[address];
        harness->events.push_back(Event('R', address, value));
        return value;
    }

    static void write(void* opaque, unsigned short address, unsigned char value)
    {
        Harness* harness = static_cast<Harness*>(opaque);
        harness->memory[address] = value;
        harness->events.push_back(Event('W', address, value));
    }

    static unsigned char input(void* opaque, unsigned short port)
    {
        (void)opaque;
        (void)port;
        return 0xFF;
    }

    static void output(void* opaque, unsigned short port, unsigned char value)
    {
        (void)opaque;
        (void)port;
        (void)value;
    }

    static void consumeClock(void* opaque, int clocks)
    {
        Harness* harness = static_cast<Harness*>(opaque);
        harness->clockTotal += clocks;
    }

    static unsigned char acknowledge(void* opaque)
    {
        Harness* harness = static_cast<Harness*>(opaque);
        unsigned char value = harness->acknowledgeFromAccumulator ? harness->cpu->reg.pair.A : harness->acknowledgeValue;
        harness->acknowledgeCount++;
        harness->events.push_back(Event('A', 0, value));
        return value;
    }
};

struct Fixture {
    Harness harness;
    Z80 cpu;

    explicit Fixture(bool installAcknowledge = true)
        : harness(), cpu(Harness::read, Harness::write, Harness::input, Harness::output, &harness)
    {
        harness.cpu = &cpu;
        cpu.setConsumeClockCallback(Harness::consumeClock);
        if (installAcknowledge) {
            cpu.setInterruptAcknowledgeCallback(Harness::acknowledge);
        }
    }
};

#define CHECK(condition)                                                                      \
    do {                                                                                      \
        if (!(condition)) {                                                                   \
            std::cerr << "check failed at line " << __LINE__ << ": " #condition << std::endl; \
            return false;                                                                     \
        }                                                                                     \
    } while (false)

static std::vector<unsigned short> readAddresses(const Harness& harness)
{
    std::vector<unsigned short> result;
    for (std::vector<Event>::const_iterator it = harness.events.begin(); it != harness.events.end(); ++it) {
        if (it->type == 'R') {
            result.push_back(it->address);
        }
    }
    return result;
}

static void armLevelIRQ(Fixture& fixture, unsigned char interruptMode, unsigned char acknowledgeValue)
{
    fixture.cpu.reg.PC = kPC;
    fixture.cpu.reg.SP = kSP;
    fixture.cpu.reg.IFF = kIFF1;
    fixture.cpu.reg.interrupt = interruptMode;
    fixture.harness.memory[kPC] = 0x00;
    fixture.harness.acknowledgeValue = acknowledgeValue;
    fixture.cpu.setIRQLine(true);
}

static bool testIRQLineState()
{
    Fixture fixture;
    CHECK(!fixture.cpu.isIRQLineAsserted());
    fixture.cpu.setIRQLine(true);
    CHECK(fixture.cpu.isIRQLineAsserted());
    fixture.cpu.setIRQLine(false);
    CHECK(!fixture.cpu.isIRQLineAsserted());
    fixture.cpu.setIRQLine(true);
    CHECK(fixture.cpu.isIRQLineAsserted());
    return true;
}

static bool testDIBlocksAcceptance()
{
    Fixture fixture;
    armLevelIRQ(fixture, 1, 0x55);
    fixture.harness.memory[kPC] = 0xF3;
    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 0);
    CHECK(fixture.cpu.isIRQLineAsserted());
    CHECK((fixture.cpu.reg.IFF & kIFF1) == 0);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 1));
    return true;
}

static bool testEIDelay()
{
    Fixture fixture;
    fixture.cpu.reg.PC = kPC;
    fixture.cpu.reg.SP = kSP;
    fixture.cpu.reg.interrupt = 1;
    fixture.harness.memory[kPC] = 0xFB;
    fixture.harness.memory[kPC + 1] = 0x00;
    fixture.cpu.setIRQLine(true);

    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 0);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 1));

    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK(fixture.cpu.reg.PC == 0x0038);
    return true;
}

static bool testAcknowledgeValueAfterEIDelay()
{
    Fixture fixture;
    fixture.cpu.reg.PC = kPC;
    fixture.cpu.reg.SP = kSP;
    fixture.cpu.reg.interrupt = 0;
    fixture.harness.memory[kPC] = 0xFB;
    fixture.harness.memory[kPC + 1] = 0x3E;
    fixture.harness.memory[kPC + 2] = 0xCF;
    fixture.harness.acknowledgeFromAccumulator = true;
    fixture.cpu.setIRQLine(true);

    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 0);
    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK(fixture.cpu.reg.pair.A == 0xCF);
    CHECK(fixture.cpu.reg.PC == 0x0008);
    CHECK(fixture.harness.memory[kSP - 1] == 0x10);
    CHECK(fixture.harness.memory[kSP - 2] == 0x03);
    return true;
}

static bool testDeassertBeforeAcceptance()
{
    Fixture fixture;
    fixture.cpu.reg.PC = kPC;
    fixture.cpu.reg.SP = kSP;
    fixture.cpu.reg.interrupt = 1;
    fixture.harness.memory[kPC] = 0xFB;
    fixture.harness.memory[kPC + 1] = 0x00;
    fixture.cpu.setIRQLine(true);

    fixture.cpu.execute(1);
    fixture.cpu.setIRQLine(false);
    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 0);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 2));
    return true;
}

static bool testPersistentAssertion()
{
    Fixture fixture;
    armLevelIRQ(fixture, 1, 0x12);
    fixture.harness.memory[0x0038] = 0xFB;
    fixture.harness.memory[0x0039] = 0x00;

    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK(fixture.cpu.isIRQLineAsserted());
    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 1);
    fixture.cpu.execute(1);
    CHECK(fixture.harness.acknowledgeCount == 2);
    CHECK(fixture.cpu.reg.PC == 0x0038);
    CHECK(fixture.cpu.isIRQLineAsserted());
    return true;
}

static bool testIM0RST()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xCF);
    fixture.harness.memory[kPC + 1] = 0x76;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK(fixture.cpu.reg.PC == 0x0008);
    CHECK(fixture.cpu.reg.SP == static_cast<unsigned short>(kSP - 2));
    CHECK(fixture.harness.memory[kSP - 1] == 0x10);
    CHECK(fixture.harness.memory[kSP - 2] == 0x01);
    CHECK(reads.size() == 1U);
    CHECK(reads[0] == kPC);
    CHECK(fixture.harness.clockTotal == 17);
    return true;
}

static bool testIM0NOP()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0x00);
    fixture.harness.memory[kPC + 1] = 0x76;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 1));
    CHECK(reads.size() == 1U);
    CHECK(reads[0] == kPC);
    CHECK(fixture.harness.clockTotal == 10);
    return true;
}

static bool testIM0LD_A_A()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0x7F);
    fixture.cpu.reg.pair.A = 0x5A;
    fixture.harness.memory[kPC + 1] = 0x76;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.pair.A == 0x5A);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 1));
    CHECK(reads.size() == 1U);
    CHECK(fixture.harness.clockTotal == 10);
    return true;
}

static bool testIM0CALL()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xCD);
    fixture.harness.memory[kPC + 1] = 0x34;
    fixture.harness.memory[kPC + 2] = 0x12;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.PC == 0x1234);
    CHECK(fixture.harness.memory[kSP - 1] == 0x10);
    CHECK(fixture.harness.memory[kSP - 2] == 0x03);
    CHECK(reads.size() == 3U);
    CHECK(reads[0] == kPC);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    CHECK(reads[2] == static_cast<unsigned short>(kPC + 2));
    CHECK(fixture.harness.clockTotal == 23);
    return true;
}

static bool testIM0CB()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xCB);
    fixture.cpu.reg.pair.B = 0x81;
    fixture.harness.memory[kPC + 1] = 0x00;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.pair.B == 0x03);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 2));
    CHECK(reads.size() == 2U);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    return true;
}

static bool testIM0ED()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xED);
    fixture.cpu.reg.pair.A = 0x01;
    fixture.harness.memory[kPC + 1] = 0x44;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.pair.A == 0xFF);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 2));
    CHECK(reads.size() == 2U);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    return true;
}

static bool testIM0DD()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xDD);
    fixture.harness.memory[kPC + 1] = 0x21;
    fixture.harness.memory[kPC + 2] = 0x34;
    fixture.harness.memory[kPC + 3] = 0x12;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.IX == 0x1234);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 4));
    CHECK(reads.size() == 4U);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    CHECK(reads[2] == static_cast<unsigned short>(kPC + 2));
    CHECK(reads[3] == static_cast<unsigned short>(kPC + 3));
    return true;
}

static bool testIM0FD()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xFD);
    fixture.harness.memory[kPC + 1] = 0x21;
    fixture.harness.memory[kPC + 2] = 0x78;
    fixture.harness.memory[kPC + 3] = 0x56;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.IY == 0x5678);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 4));
    CHECK(reads.size() == 4U);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    CHECK(reads[2] == static_cast<unsigned short>(kPC + 2));
    CHECK(reads[3] == static_cast<unsigned short>(kPC + 3));
    return true;
}

static bool testIM0DDCB()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xDD);
    fixture.cpu.reg.IX = 0x2000;
    fixture.harness.memory[kPC + 1] = 0xCB;
    fixture.harness.memory[kPC + 2] = 0x02;
    fixture.harness.memory[kPC + 3] = 0x46;
    fixture.harness.memory[0x2002] = 0x01;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 4));
    CHECK(reads.size() == 5U);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    CHECK(reads[2] == static_cast<unsigned short>(kPC + 2));
    CHECK(reads[3] == static_cast<unsigned short>(kPC + 3));
    CHECK(reads[4] == 0x2002);
    return true;
}

static bool testIM0FDCB()
{
    Fixture fixture;
    armLevelIRQ(fixture, 0, 0xFD);
    fixture.cpu.reg.IY = 0x2100;
    fixture.harness.memory[kPC + 1] = 0xCB;
    fixture.harness.memory[kPC + 2] = 0xFF;
    fixture.harness.memory[kPC + 3] = 0x86;
    fixture.harness.memory[0x20FF] = 0xFF;
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.cpu.reg.PC == static_cast<unsigned short>(kPC + 4));
    CHECK(fixture.harness.memory[0x20FF] == 0xFE);
    CHECK(reads.size() == 5U);
    CHECK(reads[1] == static_cast<unsigned short>(kPC + 1));
    CHECK(reads[2] == static_cast<unsigned short>(kPC + 2));
    CHECK(reads[3] == static_cast<unsigned short>(kPC + 3));
    CHECK(reads[4] == 0x20FF);
    return true;
}

static bool testIM1AcknowledgeIgnored()
{
    Fixture fixture;
    armLevelIRQ(fixture, 1, 0xCD);
    fixture.cpu.execute(1);

    std::vector<unsigned short> reads = readAddresses(fixture.harness);
    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK(fixture.cpu.reg.PC == 0x0038);
    CHECK(reads.size() == 1U);
    CHECK(reads[0] == kPC);
    return true;
}

static bool testIM2AcknowledgeAndVectorOrder()
{
    Fixture fixture;
    armLevelIRQ(fixture, 2, 0x34);
    fixture.cpu.reg.I = 0x20;
    fixture.harness.memory[0x2034] = 0x78;
    fixture.harness.memory[0x2035] = 0x56;
    fixture.cpu.execute(1);

    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK(fixture.cpu.reg.PC == 0x5678);
    CHECK(fixture.harness.events.size() == 6U);
    CHECK(fixture.harness.events[0].type == 'R');
    CHECK(fixture.harness.events[0].address == kPC);
    CHECK(fixture.harness.events[1].type == 'A');
    CHECK(fixture.harness.events[2].type == 'W');
    CHECK(fixture.harness.events[2].address == static_cast<unsigned short>(kSP - 1));
    CHECK(fixture.harness.events[3].type == 'W');
    CHECK(fixture.harness.events[3].address == static_cast<unsigned short>(kSP - 2));
    CHECK(fixture.harness.events[4].type == 'R');
    CHECK(fixture.harness.events[4].address == 0x2034);
    CHECK(fixture.harness.events[5].type == 'R');
    CHECK(fixture.harness.events[5].address == 0x2035);
    return true;
}

static bool testHALTWake()
{
    Fixture fixture;
    armLevelIRQ(fixture, 1, 0x00);
    fixture.cpu.reg.IFF = static_cast<unsigned char>(kIFF1 | kHalt);
    fixture.cpu.execute(1);

    CHECK(fixture.harness.acknowledgeCount == 1);
    CHECK((fixture.cpu.reg.IFF & kHalt) == 0);
    CHECK(fixture.cpu.reg.PC == 0x0038);
    CHECK(fixture.harness.memory[kSP - 1] == 0x10);
    CHECK(fixture.harness.memory[kSP - 2] == 0x00);
    return true;
}

static bool testNMIDoesNotAcknowledgeIRQ()
{
    Fixture fixture;
    armLevelIRQ(fixture, 1, 0x22);
    fixture.cpu.generateNMI(0x0066);
    fixture.cpu.execute(1);

    CHECK(fixture.harness.acknowledgeCount == 0);
    CHECK(fixture.cpu.reg.PC == 0x0066);
    CHECK(fixture.cpu.isIRQLineAsserted());
    return true;
}

static bool testLegacyOneShotSemantics()
{
    Fixture accepted(false);
    accepted.cpu.reg.PC = kPC;
    accepted.cpu.reg.SP = kSP;
    accepted.cpu.reg.IFF = kIFF1;
    accepted.harness.memory[kPC] = 0x00;
    accepted.cpu.generateIRQ(7);
    accepted.cpu.execute(1);
    CHECK(accepted.cpu.reg.PC == 0x0038);
    CHECK((accepted.cpu.reg.interrupt & 0x40) == 0);
    CHECK(accepted.harness.acknowledgeCount == 0);

    Fixture cancelled(false);
    cancelled.cpu.reg.PC = kPC;
    cancelled.cpu.reg.SP = kSP;
    cancelled.cpu.reg.IFF = kIFF1;
    cancelled.harness.memory[kPC] = 0x00;
    cancelled.cpu.generateIRQ(7);
    cancelled.cpu.cancelIRQ();
    cancelled.cpu.execute(1);
    CHECK(cancelled.cpu.reg.PC == static_cast<unsigned short>(kPC + 1));
    CHECK(cancelled.cpu.reg.SP == kSP);
    return true;
}

struct TestCase {
    const char* name;
    bool (*run)();
};

int main()
{
    const TestCase tests[] = {
        {"IRQ line state", testIRQLineState},
        {"DI blocks acceptance", testDIBlocksAcceptance},
        {"EI delay", testEIDelay},
        {"acknowledge value after EI delay", testAcknowledgeValueAfterEIDelay},
        {"deassert before acceptance", testDeassertBeforeAcceptance},
        {"persistent assertion", testPersistentAssertion},
        {"IM0 RST", testIM0RST},
        {"IM0 NOP", testIM0NOP},
        {"IM0 LD A,A", testIM0LD_A_A},
        {"IM0 CALL", testIM0CALL},
        {"IM0 CB", testIM0CB},
        {"IM0 ED", testIM0ED},
        {"IM0 DD", testIM0DD},
        {"IM0 FD", testIM0FD},
        {"IM0 DDCB", testIM0DDCB},
        {"IM0 FDCB", testIM0FDCB},
        {"IM1 acknowledge ignored", testIM1AcknowledgeIgnored},
        {"IM2 acknowledge and vector order", testIM2AcknowledgeAndVectorOrder},
        {"HALT wake", testHALTWake},
        {"NMI does not acknowledge IRQ", testNMIDoesNotAcknowledgeIRQ},
        {"legacy one-shot semantics", testLegacyOneShotSemantics},
    };
    int failures = 0;

    for (std::size_t index = 0; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (tests[index].run()) {
            std::cout << "PASS: " << tests[index].name << std::endl;
        } else {
            std::cout << "FAIL: " << tests[index].name << std::endl;
            failures++;
        }
    }

    if (failures != 0) {
        std::cerr << failures << " focused interrupt test(s) failed" << std::endl;
        return 1;
    }
    std::cout << "All focused interrupt tests passed" << std::endl;
    return 0;
}
