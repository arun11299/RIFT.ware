
# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#

from rwmgmtapi.app import app
import os
import os.path


@app.route('/')
def root():
    return app.send_static_file('index.html')

@app.route('/launchpad/config')
def static_config():
   return artifact_resource('launchpad/config/static-config.json')

def artifact_resource(name):
    if 'RIFT_ARTIFACTS' not in os.environ:
        return 'RIFT_ARTIFACTS environment variable not found', 404

    artifacts_dir = os.environ['RIFT_ARTIFACTS']
    resource_path = artifacts_dir + '/' + name
    if not os.path.isfile(resource_path):
        return 'Artifact %s found' % resource_path, 404

    try:
        with open(resource_path, 'r') as content:
            return content.read()
    except IOError as e:
        return 'Error loading %s : %s' % (resource_path, e.message), 500
