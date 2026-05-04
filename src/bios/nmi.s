.import _nmi_hook
.export _nmi_int

_nmi_int:
    jsr _nmi_hook
