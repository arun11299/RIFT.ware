
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from gi.repository import (
        NsdYang,
        RwYang,
        VldYang,
        VnfdYang,
        )


class GenericYangConverter(object):
    model = None

    def __init__(self):
        cls = self.__class__

        if cls.model is None:
            cls.model = RwYang.model_create_libncx()
            cls.model.load_schema_ypbc(cls.yang_namespace().get_schema())

    @classmethod
    def yang_namespace(cls):
        return cls.YANG_NAMESPACE

    @classmethod
    def yang_class(cls):
        return cls.YANG_CLASS

    def from_xml_string(self, xml):
        cls = self.__class__
        obj = cls.yang_class()()
        obj.from_xml_v2(cls.model, xml)
        return obj

    def from_xml_file(self, filename):
        with open(filename, 'r') as fp:
            xml = fp.read()

        cls = self.__class__
        obj = cls.yang_class()()
        obj.from_xml_v2(cls.model, xml)
        return obj

    def to_xml_string(self, obj):
        return obj.to_xml_v2(self.__class__.model)


class VnfdYangConverter(GenericYangConverter):
    YANG_NAMESPACE = VnfdYang
    YANG_CLASS = VnfdYang.YangData_Vnfd_VnfdCatalog_Vnfd


class NsdYangConverter(GenericYangConverter):
    YANG_NAMESPACE = NsdYang
    YANG_CLASS = NsdYang.YangData_Nsd_NsdCatalog_Nsd


class VldYangConverter(GenericYangConverter):
    YANG_NAMESPACE = VldYang
    YANG_CLASS = VldYang.YangData_Vld_VldCatalog_Vld
