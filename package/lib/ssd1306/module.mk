INCLUDE += -I$(SSD1306_BASE)
VPATH   += $(SSD1306_BASE)
DEFINE	+= -DSUPPORT_SSD1306

CSRCS += \
	oled96.c\
	fonts.c
