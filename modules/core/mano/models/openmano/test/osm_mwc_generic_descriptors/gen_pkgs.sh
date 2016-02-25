#!/bin/bash

tmp_dir=$(mktemp -d)
echo "Generating packages in temporary directory: ${tmp_dir}"

#6WindTR1.1.2 VNF
mkdir -p ${tmp_dir}/6wind_vnf/vnfd
cp -f rift_vnfs/6WindTR1.1.2.xml ${tmp_dir}/6wind_vnf/vnfd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} 6wind_vnf


# mwc16-pe.yaml
mkdir -p ${tmp_dir}/mwc16_pe_ns/nsd
cp -f rift_scenarios/mwc16-pe.xml ${tmp_dir}/mwc16_pe_ns/nsd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} mwc16_pe_ns

# tidgen_mwc16_vnf.yaml
mkdir -p ${tmp_dir}/tidgen_mwc16_vnf/vnfd
cp -f rift_vnfs/mwc16gen1.xml ${tmp_dir}/tidgen_mwc16_vnf/vnfd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} tidgen_mwc16_vnf

# mwc16-gen.yaml
mkdir -p ${tmp_dir}/mwc16_gen_ns/nsd
cp -f rift_scenarios/mwc16-gen.xml ${tmp_dir}/mwc16_gen_ns/nsd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} mwc16_gen_ns


# IMS-ALLin1_2p.yaml
mkdir -p ${tmp_dir}/ims_allin1_2p_vnf/vnfd
cp -f rift_vnfs/IMS-ALLIN1.xml ${tmp_dir}/ims_allin1_2p_vnf/vnfd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} ims_allin1_2p_vnf

# IMS-allin1-corpa.yaml
mkdir -p ${tmp_dir}/ims_allin1_corpa/nsd
cp -f rift_scenarios/IMS-corpA.xml ${tmp_dir}/ims_allin1_corpa/nsd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} ims_allin1_corpa


# gw_corpA_PE1.yaml
mkdir -p ${tmp_dir}/gw_corpa_pe1_vnf/vnfd
cp -f rift_vnfs/gw-corpa-pe1.xml ${tmp_dir}/gw_corpa_pe1_vnf/vnfd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} gw_corpa_pe1_vnf

# gw_corpA_PE2.yaml
mkdir -p ${tmp_dir}/gw_corpa_pe2_vnf/vnfd
cp -f rift_vnfs/gw-corpa-pe2.xml ${tmp_dir}/gw_corpa_pe2_vnf/vnfd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} gw_corpa_pe2_vnf

# gw_corpa_ns.yaml
mkdir -p ${tmp_dir}/gw_corpa_ns/nsd
cp -f rift_scenarios/gwcorpA.xml ${tmp_dir}/gw_corpa_ns/nsd
${RIFT_ROOT}/bin/generate_descriptor_pkg.sh ${tmp_dir} gw_corpa_ns
