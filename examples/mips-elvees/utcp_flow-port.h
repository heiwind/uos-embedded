#ifndef __UTCP_FLOW_PORT_H
#define __UTCP_FLOW_PORT_H

#include <stdint.h>
#include <runtime/sys/uosc.h>
#include <test-utcp-datasource.h>

//*     virtual class of interface to Raw bricks provider
//*     and you need define this methods implementation
class utcp_dma_source {
public:
    typedef uint32_t RawSampleId;
    typedef uint32_t ChMask;
    typedef uint16_t raw_sample;

    typedef void* Brick;
    struct SoftBrick{
          int       pos;
          size_t    size;
          size_t    limit;
          Brick     data;
      };

    class BrickAllocator{
        protected:
            size_t page_size;
        public:
        BrickAllocator(){};
        virtual ~BrickAllocator(){};

        size_t pagesize(void){return page_size;};

        //уведомление о том сколько предстоит размещать кирпичей
        //\return - сколько кирпичей досутпно
        virtual unsigned            Declare(size_t brick_size, unsigned bricks){ return bricks;};
        virtual Brick               Alloc(size_t size){return 0;};
        virtual void                Free(Brick x, size_t size){};

    };

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
  virtual ~utcp_dma_source() __noexcept __NOTHROW {};
};



#include <buffers/ring_index.h>
class RawSource : public utcp_dma_source {
    public:
        typedef utcp_dma_source inherited;

    protected:
        static const unsigned   bricks_limit    = 128; 
        static const unsigned   sample_size     = 512;
        static const unsigned   frame_size      = sample_size*16;
        unsigned                brick_size;
        ring_uindex_t           brick_idx;
        Brick                   bricks[bricks_limit];
        SoftBrick               _ReadBrick;
        unsigned                fill_brick(Brick dst, unsigned stamp);

    public:
      RawSource();
      virtual ~RawSource() __noexcept __NOTHROW;

      unsigned                tail_stamp;
      unsigned                ReadIdx;

      bool AtRead(SoftBrick* dst, unsigned next_no);
      bool NextRead();         //отпускает текущий блок, берет следующий
      RawSampleId ReadSample() {return tail_stamp;};
      unsigned sizeof_frame(unsigned mss);

      //unsigned sizeof_frame()  const {return frame_size;};
      //unsigned sizeof_sample() const {return sample_size;};
      //unsigned BlockSamples()  const {return brick_size/sizeof(long);};

    public:

      class BrickAllocator: public inherited::BrickAllocator{
          // этот аллокатор картирует буферы измеряки в очередь кирпичей ФИФО
          // он же должен заниматься распределением банков по трекам каналов и настройкой
          //    измеряки
          public:
              typedef RawSource::inherited::BrickAllocator inherited;
              typedef RawSource::Brick  Brick;
              typedef RawSource::ChMask ChMask;
              static const unsigned long brickNULL = (~0);
              static bool  isNULL(Brick p) {return p == (Brick)~0;};

              static const unsigned sample_len = 256;
              static const unsigned ch_limit = 16;

          public:
          BrickAllocator();
          virtual ~BrickAllocator();

          //\return - сколько кирпичей досутпно
          virtual unsigned            Declare(size_t brick_size, unsigned bricks);
          unsigned                    Declare_samples(size_t brick_samples);
          // !!! возвращаемый указатель не является указателем памяти
          virtual Brick               Alloc(size_t size);
          virtual void                Free(Brick x, size_t size);

          void init_banks(size_t org, size_t size);

          protected:
              ChMask    _chs;
              unsigned  chs_count;
              //это карта банков ДСП
              //банк ДСП кодирую младшим битом адреса кирпича.!!! это надо
              //    учитывать при доступе к данным
              static const unsigned banks_limit = 2;
              struct trek_info {
                  unsigned  ch;
                  size_t  sizeof_sample;
                  size_t  sizeof_brick;
              } trek[ch_limit];
              unsigned  brick_samples;
              size_t    frame_size;
              size_t    brick_size;

              void treks_init(unsigned _brick_samples);

              struct dsp_bank {
                  uintptr_t org;
                  unsigned  size;
                  unsigned  bricks;
                  unsigned  samples;
                  unsigned  alloc_sample;
                  // = bank_samples* sample_size
                  struct trek_info {
                      unsigned  org;    //absolute origin
                      size_t    size;
                  } trek[ch_limit];
                  void assign_mem(size_t org, size_t size);
                  void clear(){ size = 0; samples = 0;};
              } bank[banks_limit];
              unsigned bank_init(dsp_bank& b);

          public:

              static Brick as_brick(unsigned bank, unsigned sample) {
                  return (Brick)((sample << 1) | bank);
              };
              static unsigned sample_of(Brick x){
                  return ((unsigned)x) >> 1;
              };
              static unsigned bank_of(Brick x){
                  return ((unsigned)x) & 1;
              };
              unsigned brickidx_of(Brick x) const {
                  unsigned res = sample_of(x) / brick_samples;
                  if (bank_of(x) > 0)
                      res += bank[0].bricks;
                  return res;
              };

          public:
              void      chs(ChMask chs);
              ChMask    chs() {return _chs;};
              unsigned  chcount() {return chs_count;};
              unsigned  sizeof_sample(unsigned tid) {  return trek[tid].sizeof_sample;  };
              size_t    sizeof_frame(){return frame_size; };
              size_t    sizeof_frame(ChMask chs) {return Measures::Raw::frame_size(_chs);};
              size_t    sizeof_brick(){return brick_size;};
              unsigned  sizeof_brick_trek(unsigned tid) {  return trek[tid].sizeof_brick;};
              size_t    brick_len(){return brick_samples;};

              //ДСП рализует долбанутую технику заполнения буферов: есть несколько (2)
              //    банков. банки содержат непрерывные треки семплов на каждый канал
              //    оцифровки. семплы содержат непрерывный кусок оцифрованого потока от АЦП
              //    из 1го канала
              void* brick_sample(Brick brick, unsigned tid){
                  unsigned samp = sample_of(brick);
                  dsp_bank& b = bank[bank_of(brick)];
                  return (void*)(b.trek[tid].org + samp*sizeof_sample(tid));
              };

              size_t sizeof_trek(Brick brick, unsigned tid){
                  return bank[bank_of(brick)].trek[tid].size;
              };

              void brick_fill(Brick brick, RawSampleId x);

      }; //BrickAllocator

      BrickAllocator* pool;
      void            Pool(BrickAllocator* x);
      BrickAllocator* Pool(){return (BrickAllocator*)pool;};

      static const unsigned long brickNULL = BrickAllocator::brickNULL;
      static bool  isNULL(Brick p) {return BrickAllocator::isNULL(p);};

      public:
      class brick_navigator {
          public:
              typedef BrickAllocator MemPool;

              Brick     data;
              MemPool*  pool;

              brick_navigator(){};
              brick_navigator(Brick     x, RawSource* src) : data(x), pool(src->Pool()){};
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
      }; //brick_navigator
};

typedef class RawSource utcptx_dma_source;

#endif
  
