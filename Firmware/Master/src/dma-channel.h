#ifndef DMA_CHANNEL_included
#define DMA_CHANNEL_included

typedef struct DMA_channel_config {
    uint32_t dc_dma;
    uint8_t  dc_stream;
    uint8_t  dc_channel;
    uint32_t dc_par;
    uint32_t dc_ma0r;
    uint32_t dc_ma1r;
    uint32_t dc_ntdr;
    uint32_t dc_fcr;
} DMA_channel_config;

extern void setup_dma_channel   (const DMA_channel_config *);
extern void dma_start_transfer  (const DMA_channel_config *);
extern void dma_finish_transfer (const DMA_channel_config *);

#endif /* !DMA_CHANNEL_included */
