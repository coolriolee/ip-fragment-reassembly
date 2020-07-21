TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lpcap -ldl -lm -lpthread

SOURCES += main.c \
    ipv4-fragment.c \
    ipv4-reassembly.c \
    handler.c \
    rbtree2/key_elem.c \
    rbtree2/rbtree.c \
    rbtree2/set_elem.c \
    rbtree2/tag_elem.c

HEADERS += \
    handler.h \
    rbtree2/compiler.h \
    rbtree2/key_elem.h \
    rbtree2/rbtree.h \
    rbtree2/rbtree_augmented.h \
    rbtree2/set_elem.h \
    rbtree2/tag_elem.h
