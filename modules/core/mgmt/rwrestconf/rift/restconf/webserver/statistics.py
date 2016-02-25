# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#
# Author(s): Max Beckett
# Creation Date: 9/11/2015
# 

class Statistics(object):
    def __init__(self):
        self._del_200_rsp = 0
        self._del_404_rsp = 0
        self._del_405_rsp = 0
        self._del_500_rsp = 0
        self._del_req = 0
        self._get_200_rsp = 0
        self._get_204_rsp = 0
        self._get_404_rsp = 0
        self._get_500_rsp = 0
        self._get_req = 0
        self._post_200_rsp = 0
        self._post_201_rsp = 0
        self._post_404_rsp = 0
        self._post_405_rsp = 0
        self._post_409_rsp = 0
        self._post_500_rsp = 0
        self._post_req = 0
        self._put_200_rsp = 0
        self._put_201_rsp = 0
        self._put_404_rsp = 0
        self._put_405_rsp = 0
        self._put_409_rsp = 0
        self._put_500_rsp = 0
        self._put_req = 0


    @property
    def del_200_rsp(self):
        return self._del_200_rsp

    @del_200_rsp.setter
    def del_200_rsp(self, value):
        self._del_200_rsp = value

    @property
    def del_404_rsp(self):
        return self._del_404_rsp

    @del_404_rsp.setter
    def del_404_rsp(self, value):
        self._del_404_rsp = value

    @property
    def del_405_rsp(self):
        return self._del_405_rsp

    @del_405_rsp.setter
    def del_405_rsp(self, value):
        self._del_405_rsp = value

    @property
    def del_500_rsp(self):
        return self._del_500_rsp

    @del_500_rsp.setter
    def del_500_rsp(self, value):
        self._del_500_rsp = value

    @property
    def del_req(self):
        return self._del_req

    @del_req.setter
    def del_req(self, value):
        self._del_req = value

    @property
    def get_200_rsp(self):
        return self._get_200_rsp

    @get_200_rsp.setter
    def get_200_rsp(self, value):
        self._get_200_rsp = value

    @property
    def get_204_rsp(self):
        return self._get_204_rsp

    @get_204_rsp.setter
    def get_204_rsp(self, value):
        self._get_204_rsp = value

    @property
    def get_404_rsp(self):
        return self._get_404_rsp

    @get_404_rsp.setter
    def get_404_rsp(self, value):
        self._get_404_rsp = value

    @property
    def get_500_rsp(self):
        return self._get_500_rsp

    @get_500_rsp.setter
    def get_500_rsp(self, value):
        self._get_500_rsp = value

    @property
    def get_req(self):
        return self._get_req

    @get_req.setter
    def get_req(self, value):
        self._get_req = value

    @property
    def post_200_rsp(self):
        return self._post_200_rsp

    @post_200_rsp.setter
    def post_200_rsp(self, value):
        self._post_200_rsp = value

    @property
    def post_201_rsp(self):
        return self._post_201_rsp

    @post_201_rsp.setter
    def post_201_rsp(self, value):
        self._post_201_rsp = value

    @property
    def post_404_rsp(self):
        return self._post_404_rsp

    @post_404_rsp.setter
    def post_404_rsp(self, value):
        self._post_404_rsp = value

    @property
    def post_405_rsp(self):
        return self._post_405_rsp

    @post_405_rsp.setter
    def post_405_rsp(self, value):
        self._post_405_rsp = value

    @property
    def post_409_rsp(self):
        return self._post_409_rsp

    @post_409_rsp.setter
    def post_409_rsp(self, value):
        self._post_409_rsp = value

    @property
    def post_500_rsp(self):
        return self._post_500_rsp 

    @post_500_rsp.setter
    def post_500_rsp(self, value):
        self._post_500_rsp  = value

    @property
    def post_req(self):
        return self._post_req

    @post_req.setter
    def post_req(self, value):
        self._post_req = value

    @property
    def put_200_rsp(self):
        return self._put_200_rsp

    @put_200_rsp.setter
    def put_200_rsp(self, value):
        self._put_200_rsp = value

    @property
    def put_201_rsp(self):
        return self._put_201_rsp

    @put_201_rsp.setter
    def put_201_rsp(self, value):
        self._put_201_rsp = value

    @property
    def put_404_rsp(self):
        return self._put_404_rsp

    @put_404_rsp.setter
    def put_404_rsp(self, value):
        self._put_404_rsp = value

    @property
    def put_405_rsp(self):
        return self._put_405_rsp

    @put_405_rsp.setter
    def put_405_rsp(self, value):
        self._put_405_rsp = value

    @property
    def put_409_rsp(self):
        return self._put_409_rsp

    @put_409_rsp.setter
    def put_409_rsp(self, value):
        self._put_409_rsp = value

    @property
    def put_500_rsp(self):
        return self._put_500_rsp

    @put_500_rsp.setter
    def put_500_rsp(self, value):
        self._put_500_rsp = value

    @property
    def put_req(self):
        return self._put_req

    @put_req.setter
    def put_req(self, value):
        self._put_req = value



    
