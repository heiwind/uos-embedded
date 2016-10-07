#ifndef __UTCP_FLOW_PORT_H
#define __UTCP_FLOW_PORT_H

#include <stdint.h>
#include <runtime/sys/uosc.h>
#include <BosRawProxy.h>
#include <stdint.h>

//*     virtual class of interface to Raw bricks provider
//*     and you need define this methods implementation
class utcp_dma_source {
public:
    typedef uint32_t RawSampleId;
    typedef uint32_t ChMask;
    typedef CBosRawProxy::CBosRaw  bricks_source;

    typedef bricks_source::SoftBrick    SoftBrick;
    typedef bricks_source::Brick        Brick;

protected:
  SoftBrick*    ReadBrick;
  unsigned      _block_samples;
public:
  unsigned BlockSamples(){return _block_samples;};

  inline SoftBrick& AtRead(void){return *ReadBrick;};//at Read position

public:
  //* this methods need to be implemented by dma_flow maintainer!
  bool  AtRead(SoftBrick& dst, unsigned next_no);
  bool NextRead();         //отпускает текущий блок, берет следующий
  RawSampleId ReadSample();
  unsigned sizeof_frame(unsigned mss);

public:
  utcp_dma_source() __noexcept __NOTHROW {};
  ~utcp_dma_source() __noexcept __NOTHROW {};
};



#include <buffers/ring_index.h>
class utcp_dma_BosRaw : public utcp_dma_source {
    public:
        typedef utcp_dma_source inherited;

    protected:
        bricks_source*    raw;

    public:
      utcp_dma_BosRaw(bricks_source* src);
      ~utcp_dma_BosRaw(){};

      bool AtRead(SoftBrick* dst, unsigned next_no){return raw->AtRead(*dst, next_no);};
      bool NextRead(){return raw->NextRead();};         //отпускает текущий блок, берет следующий
      RawSampleId ReadSample() {return raw->ReadSample();};
      unsigned sizeof_frame(unsigned mss);

      //unsigned sizeof_frame()  const {return frame_size;};
      //unsigned sizeof_sample() const {return sample_size;};
      //unsigned BlockSamples()  const {return brick_size/sizeof(long);};

    public:
      class brick_navigator {
          public:
              typedef CBosRawProxy::BrickAllocator MemPool;

              Brick     data;
              MemPool*  pool;

              brick_navigator(){};
              brick_navigator(Brick     x, utcp_dma_BosRaw* src) : data(x), pool(src->raw->Pool()){};
              ~brick_navigator(){};

              typedef  unsigned inBrickPtr;

          protected:
              static const unsigned trecks_p2 = 8;
              static const unsigned trecks_limit = (1 << trecks_p2);
              static const unsigned trecks_mask = (1 << trecks_p2)-1;

              static
              unsigned treck_of(inBrickPtr p) {
                  return p & trecks_mask;
              };

              static
              unsigned ofset_of(inBrickPtr p) {
                  return p & ~trecks_mask;
              };

              static
              inBrickPtr as_ptr(unsigned treck, unsigned ofs) {
                  return ofs | treck;
              };

              unsigned treks() const {return pool->chcount();};

          public:
              void* data_at(inBrickPtr p) const {
                  uintptr_t res = (uintptr_t)pool->brick_sample(data, treck_of(p));
                  return   (void*)(res + ofset_of(p));
              };

              static const unsigned ptr_align = 0x100;
              static
              unsigned align(unsigned x) {return x & ~(ptr_align-1);};

              unsigned len_at(inBrickPtr p) const {
                  unsigned treck_len = pool->sizeof_brick_trek(treck_of(p));
                  return treck_len - ofset_of(p);
              };

              inBrickPtr succ(inBrickPtr p, unsigned ofs) const;

              static const inBrickPtr null = ~0ul;
              static
              bool  isNULL(inBrickPtr p) {return ~p == 0;};
      };
};

typedef class utcp_dma_BosRaw utcptx_dma_source;

#endif
  
