// wordはかなりノーチェック


#define	BYTE_ROL1(d, s)	{											\
		UINT tmp = ((s) >> 7);										\
		(d) = ((s) << 1) + tmp;										\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= tmp;											\
		UPD9002_OV = ((s) ^ (d)) & 0x80;								\
	}

#define BYTE_ROR1(d, s) {											\
		UINT tmp = ((s) & 1);										\
		(d) = ((tmp << 8) + (s)) >> 1;								\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= tmp;											\
		UPD9002_OV = ((s) ^ (d)) & 0x80;								\
	}

#define BYTE_RCL1(d, s)												\
		(d) = ((s) << 1) | (UPD9002_FLAGL & C_FLAG);					\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= ((s) >> 7);									\
		UPD9002_OV = ((s) ^ (d)) & 0x80;

#define	BYTE_RCR1(d, s)												\
		(d) = (((UPD9002_FLAGL & C_FLAG) << 8) | (s)) >> 1;			\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= ((s) & 1);									\
		UPD9002_OV = ((s) ^ (d)) & 0x80;

#define	BYTE_SHL1(d, s)												\
		(d) = (s) << 1;												\
		UPD9002_OV = ((s) ^ (d)) & 0x80;								\
		UPD9002_FLAGL = BYTESZPCF(d) | A_FLAG;

#define	BYTE_SHR1(d, s)												\
		(d) = (s) >> 1;												\
		UPD9002_OV = (s) & 0x80;										\
		UPD9002_FLAGL = (UINT8)(BYTESZPF(d) | A_FLAG | ((s) & 1));

#if 1
#define	BYTE_SAR1(d, s)												\
		(d) = ((s) & 0x80) + ((s) >> 1);							\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = (UINT8)(BYTESZPF(d) | A_FLAG | ((s) & 1));
#else	// eVC3/4 compiler bug
#define	BYTE_SAR1(d, s)												\
		(d) = (BYTE)(((SINT8)(s)) >> 1);							\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = (UINT8)(BYTESZPF(d) | A_FLAG | ((s) & 1));
#endif


#define	WORD_ROL1(d, s)	{											\
		UINT32 tmp = ((s) >> 15);									\
		(d) = ((s) << 1) + tmp;										\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= tmp;											\
		UPD9002_OV = ((s) ^ (d)) & 0x8000;								\
	}

#define WORD_ROR1(d, s) {											\
		UINT32 tmp = ((s) & 1);										\
		(d) = ((tmp << 16) + (s)) >> 1;								\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= tmp;											\
		UPD9002_OV = ((s) ^ (d)) & 0x8000;								\
	}

#define WORD_RCL1(d, s)												\
		(d) = ((s) << 1) | (UPD9002_FLAGL & 1);						\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= ((s) >> 15);									\
		UPD9002_OV = ((s) ^ (d)) & 0x8000;

#define	WORD_RCR1(d, s)												\
		(d) = (((UPD9002_FLAGL & 1) << 16) + (s)) >> 1;				\
		UPD9002_FLAGL &= ~C_FLAG;										\
		UPD9002_FLAGL |= ((s) & 1);									\
		UPD9002_OV = ((s) ^ (d)) & 0x8000;

#define	WORD_SHL1(d, s)												\
		(d) = (s) << 1;												\
		UPD9002_OV = ((s) ^ (d)) & 0x8000;								\
		UPD9002_FLAGL = WORDSZPCF(d) + A_FLAG;

#define	WORD_SHR1(d, s)												\
		(d) = (s) >> 1;												\
		UPD9002_OV = (s) & 0x8000;										\
		UPD9002_FLAGL = (UINT8)(WORDSZPF(d) | A_FLAG | ((s) & 1));

#if 1
#define	WORD_SAR1(d, s)												\
		(d) = ((s) & 0x8000) + ((s) >> 1);							\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = (UINT8)(WORDSZPF(d) | A_FLAG | ((s) & 1));
#else	// eVC3/4 compiler bug
#define	WORD_SAR1(d, s)												\
		(d) = (UINT16)(((SINT16)(s)) >> 1);							\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = (UINT8)(WORDSZPF(d) | A_FLAG | ((s) & 1));
#endif



#define	BYTE_ROLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(c) = ((c) - 1) & 7;									\
			if (c) {												\
				(s) = ((s) << (c)) | ((s) >> (8 - (c)));			\
				(s) &= 0xff;										\
			}														\
			BYTE_ROL1(d, s)											\
		}															\
		else {														\
			(d) = (s);												\
		}

#define	BYTE_RORCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(c) = ((c) - 1) & 7;									\
			if (c) {												\
				(s) = ((s) >> (c)) | ((s) << (8 - (c)));			\
				(s) &= 0xff;										\
			}														\
			BYTE_ROR1(d, s)											\
		}															\
		else {														\
			(d) = (s);												\
		}

#define	BYTE_RCLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			tmp = UPD9002_FLAGL & C_FLAG;								\
			UPD9002_FLAGL &= ~C_FLAG;									\
			while((c)--) {											\
				(s) = (((s) << 1) | tmp) & 0x1ff;					\
				tmp = (s) >> 8;										\
			}														\
			UPD9002_OV = ((s) ^ (s >> 1)) & 0x80;						\
			UPD9002_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	BYTE_RCRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			tmp = UPD9002_FLAGL & C_FLAG;								\
			UPD9002_FLAGL &= ~C_FLAG;									\
			while((c)--) {											\
				(s) |= tmp << 8;									\
				tmp = (s) & 1;										\
				(s) >>= 1;											\
			}														\
			UPD9002_OV = ((s) ^ (s >> 1)) & 0x40;						\
			UPD9002_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	BYTE_SHLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			if ((c) > 10) {											\
				(c) = 10;											\
			}														\
			(s) <<= (c);											\
			(s) &= 0x1ff;											\
			UPD9002_FLAGL = BYTESZPCF(s) + A_FLAG;						\
			UPD9002_OV = ((s) ^ ((s) >> 1)) & 0x80;					\
		}															\
		(d) = (s);

#define	BYTE_SHRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			if ((c) >= 10) {										\
				(c) = 10;											\
			}														\
			(s) >>= ((c) - 1);										\
			UPD9002_FLAGL = (BYTE)((s) & 1);							\
			(s) >>= 1;												\
			UPD9002_OV = ((s) ^ ((s) >> 1)) & 0x40;					\
			UPD9002_FLAGL |= BYTESZPF(s) + A_FLAG;						\
		}															\
		(d) = (s);

#if !defined(_WIN32_WCE) || (_WIN32_WCE < 300)
#define	BYTE_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(s) = ((SINT8)(s)) >> ((c) - 1);						\
			UPD9002_FLAGL = (BYTE)((s) & 1);							\
			(s) = (BYTE)(((SINT8)s) >> 1);							\
			UPD9002_OV = 0;											\
			UPD9002_FLAGL |= BYTESZPF(s) | A_FLAG;						\
		}															\
		(d) = (s);
#else
#define	BYTE_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			SINT32 t;												\
			t = (s) << 24;											\
			t = t >> ((c) - 1);										\
			UPD9002_FLAGL = (UINT8)((t >> 24) & 1);					\
			(s) = (t >> 25) & 0xff;									\
			UPD9002_OV = 0;											\
			UPD9002_FLAGL |= BYTESZPF(s) | A_FLAG;						\
		}															\
		(d) = (s);
#endif

#define	WORD_ROLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			(c)--;													\
			if (c) {												\
				(c) &= 0x0f;										\
				(s) = ((s) << (c)) | ((s) >> (16 - (c)));			\
				(s) &= 0xffff;										\
			}														\
			else {													\
				UPD9002_OV = ((s) + 0x4000) & 0x8000;					\
			}														\
			tmp = ((s) >> 15);										\
			(s) = ((s) << 1) + tmp;									\
			UPD9002_FLAGL &= ~C_FLAG;									\
			UPD9002_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_RORCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT32 tmp;												\
			(c)--;													\
			if (c) {												\
				(c) &= 0x0f;										\
				(s) = ((s) >> (c)) | ((s) << (16 - (c)));			\
				(s) &= 0xffff;										\
			}														\
			else {													\
				UPD9002_OV = ((s) >> 15) ^ ((s) & 1);					\
			}														\
			tmp = (s) & 1;											\
			(s) = ((tmp << 16) + (s)) >> 1;							\
			UPD9002_FLAGL &= ~C_FLAG;									\
			UPD9002_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_RCLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			tmp = UPD9002_FLAGL & C_FLAG;								\
			UPD9002_FLAGL &= ~C_FLAG;									\
			if ((c) == 1) {											\
				UPD9002_OV = ((s) + 0x4000) & 0x8000;					\
			}														\
			while((c)--) {											\
				(s) = (((s) << 1) + tmp) & 0x1ffff;					\
				tmp = (s) >> 16;									\
			}														\
			UPD9002_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_RCRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT32 tmp;												\
			tmp = UPD9002_FLAGL & C_FLAG;								\
			UPD9002_FLAGL &= ~C_FLAG;									\
			if ((c) == 1) {											\
				UPD9002_OV = ((s) >> 15) ^ tmp;						\
			}														\
			while((c)--) {											\
				(s) |= tmp << 16;									\
				tmp = (s) & 1;										\
				(s) >>= 1;											\
			}														\
			UPD9002_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_SHLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UPD9002_OV = 0;											\
			if ((c) == 1) {											\
				UPD9002_OV = ((s) + 0x4000) & 0x8000;					\
			}														\
			(s) <<= (c);											\
			(s) &= 0x1ffff;											\
			UPD9002_FLAGL = WORDSZPCF(s);								\
		}															\
		(d) = (s);

#define	WORD_SHRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(c)--;													\
			if (c) {												\
				(s) >>= (c);										\
				UPD9002_OV = 0;										\
			}														\
			else {													\
				UPD9002_OV = (s) & 0x8000;								\
			}														\
			UPD9002_FLAGL = (UINT8)((s) & 1);							\
			(s) >>= 1;												\
			UPD9002_FLAGL |= WORDSZPF(s);								\
		}															\
		(d) = (s);

#if !defined(_WIN32_WCE) || (_WIN32_WCE < 300)
#define	WORD_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(s) = ((SINT16)(s)) >> ((c) - 1);						\
			UPD9002_FLAGL = (UINT8)((s) & 1);							\
			(s) = (UINT16)(((SINT16)s) >> 1);						\
			UPD9002_OV = 0;											\
			UPD9002_FLAGL |= WORDSZPF(s);								\
		}															\
		(d) = (s);
#else	// eVC～
#define	WORD_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			SINT32 tmp;												\
			tmp = (s) << 16;										\
			tmp = tmp >> (16 + (c) - 1);							\
			UPD9002_FLAGL = (UINT8)(tmp & 1);							\
			(s) = (UINT16)(tmp >> 1);								\
			UPD9002_OV = 0;											\
			UPD9002_FLAGL |= WORDSZPF(s);								\
		}															\
		(d) = (s);
#endif

