
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

cmd npm set registry http://npm.eng.riftio.com:4873/
cmd npm install -g --production forever
cmd systemctl enable salt-master
cmd systemctl enable libvirtd
