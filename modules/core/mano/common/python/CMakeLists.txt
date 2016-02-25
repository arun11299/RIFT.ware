# Creation Date: 2016/1/12
# RIFT_IO_STANDARD_CMAKE_COPYRIGHT_HEADER(END)

cmake_minimum_required(VERSION 2.8)


rift_python_install_tree(
  FILES
    rift/mano/cloud/__init__.py
    rift/mano/cloud/accounts.py
    rift/mano/cloud/config.py
    rift/mano/cloud/operdata.py
    rift/mano/config_agent/operdata.py
    rift/mano/config_agent/__init__.py
    rift/mano/config_agent/config.py
  COMPONENT ${PKG_LONG_NAME}
  PYTHON3_ONLY
  )
