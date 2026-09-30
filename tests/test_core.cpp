// Headless unit tests for the CHIP-8 core.
#define private public   // tests inspect internal registers
#include "../src/chip8.h"
#undef private
#include <cstdio>
#include <cstring>

static int fails = 0, total = 0;
#define CHECK(cond, msg) do{ total++; if(!(cond)){ fails++; printf("FAIL: %s\n", msg);} }while(0)

// Put a program at 0x200 and run n instructions
static void run(Chip8& c, std::initializer_list<uint16_t> prog, int n){
    int a = 0x200;
    for(uint16_t op : prog){ c.memory[a++] = op >> 8; c.memory[a++] = op & 0xFF; }
    c.pc = 0x200;
    for(int i=0; i<n; i++) c.emulate_cycle();
}

int main(){
    { Chip8 c; // 2nnn / 00EE round trip
      run(c, {0x2204, 0x1202, 0x00EE}, 2); // call 0x204, return
      CHECK(c.pc == 0x202 && c.sp == 0, "00EE returns to caller+2 and sp balanced"); }

    { Chip8 c; run(c, {0x60FF, 0x6101, 0x8014}, 3); // 255+1
      CHECK(c.v[0]==0 && c.v[0xF]==1, "8xy4 carry"); }
    { Chip8 c; run(c, {0x6005, 0x6105, 0x8015}, 3); // 5-5 : no borrow => VF=1
      CHECK(c.v[0]==0 && c.v[0xF]==1, "8xy5 equal operands => VF=1"); }
    { Chip8 c; run(c, {0x6003, 0x6105, 0x8015}, 3); // 3-5 : borrow => VF=0
      CHECK(c.v[0]==0xFE && c.v[0xF]==0, "8xy5 borrow"); }
    { Chip8 c; run(c, {0x6003, 0x6105, 0x8017}, 3); // 5-3
      CHECK(c.v[0]==2 && c.v[0xF]==1, "8xy7"); }
    { Chip8 c; run(c, {0x6003, 0x8006}, 2);
      CHECK(c.v[0]==1 && c.v[0xF]==1, "8xy6 shr + LSB"); }
    { Chip8 c; run(c, {0x6081, 0x800E}, 2);
      CHECK(c.v[0]==2 && c.v[0xF]==1, "8xyE shl + MSB"); }
    { Chip8 c; run(c, {0x6F01, 0x8FF6}, 2); // x==F: flag must win
      CHECK(c.v[0xF]==1, "8FF6 flag written last"); }

    { Chip8 c; c.index=0x300; run(c, {0x60FE, 0xF033}, 2); // 254
      CHECK(c.memory[0x300]==2 && c.memory[0x301]==5 && c.memory[0x302]==4, "Fx33 BCD of 254"); }
    { Chip8 c; c.index=0x300; run(c, {0x6001,0x6102,0x6203, 0xF255}, 4);
      CHECK(c.memory[0x300]==1 && c.memory[0x301]==2 && c.memory[0x302]==3, "Fx55 stores V0..Vx inclusive"); }
    { Chip8 c; c.index=0x300; c.memory[0x300]=7; c.memory[0x301]=8; c.memory[0x302]=9;
      run(c, {0xF265}, 1);
      CHECK(c.v[0]==7 && c.v[1]==8 && c.v[2]==9, "Fx65 loads V0..Vx inclusive"); }

    { Chip8 c; run(c, {0xF10A}, 3); // no key: must block on same pc
      CHECK(c.pc == 0x200, "Fx0A blocks while no key"); 
      c.key[7]=1; c.emulate_cycle();
      CHECK(c.pc == 0x202 && c.v[1]==7, "Fx0A continues once key pressed"); }

    { Chip8 c; run(c, {0x6005, 0xF015, 0x6000, 0x6000}, 4); // 4 CPU cycles must NOT drain timer
      CHECK(c.delay_timer == 5, "timer unaffected by CPU cycles");
      c.update_timers(); CHECK(c.delay_timer == 4, "update_timers ticks once"); }

    { Chip8 c; run(c, {0x00E0, 0xA000, 0x6000, 0x6100, 0xD015}, 5); // draw font '0' at (0,0)
      CHECK(c.display[0]==1 && c.display[1]==1 && c.display[4]==0, "Dxyn draws font glyph top row (F0)"); 
      CHECK(c.v[0xF]==0, "no collision first draw");
      c.pc=0x208; c.emulate_cycle(); CHECK(c.v[0xF]==1 && c.display[0]==0, "second draw XORs off + collision"); }

    { Chip8 a, b; a.index=0x123; a.v[3]=42; a.pc=0x456; a.sp=2; a.stack[1]=0x321; a.delay_timer=9; a.sound_timer=4; a.display[100]=1; a.memory[0x777]=0xAB; a.key[5]=1;
      CHECK(a.save_state("test_state.c8s"), "save_state ok");
      CHECK(b.load_state("test_state.c8s"), "load_state ok");
      CHECK(b.index==0x123 && b.v[3]==42 && b.pc==0x456 && b.sp==2 && b.stack[1]==0x321 &&
            b.delay_timer==9 && b.sound_timer==4 && b.display[100]==1 && b.memory[0x777]==0xAB && b.key[5]==1, "savestate round-trips every field");
      remove("test_state.c8s"); }
    { Chip8 c; c.pc=0x300;
      FILE* f=fopen("bad_state.c8s","wb"); fwrite("garbage",1,7,f); fclose(f);
      CHECK(!c.load_state("bad_state.c8s") && c.pc==0x300, "corrupt file rejected, state untouched");
      remove("bad_state.c8s");
      CHECK(!c.load_state("non_existent_file.c8s"), "missing file rejected"); }

    printf("%d/%d checks passed\n", total-fails, total);
    return fails ? 1 : 0;
}
