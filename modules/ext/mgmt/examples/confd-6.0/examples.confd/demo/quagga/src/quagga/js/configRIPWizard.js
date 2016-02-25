ConfigRIPWizard = {
  passwd: 'SecretPassword',
  ifname: 'eeth0',
  netmask: '10.4.0.0',
  preflen: '24',

  show: function(currentMainContentDiv) {
    if (currentMainContentDiv != null)
      currentMainContentDiv.style.display = 'none';

    CustomContent.show(null);
    ConfigRIPWizard.gotoStep1();
    return $('customContent');
  },

  gotoStep1: function() {
    var url = ConfD.baseUrl+'/configRIPWizard1.html';
    var updatePage = function() {
      if (ConfigRIPWizard.ifname)
        $('ifname').value = ConfigRIPWizard.ifname;

      if (ConfigRIPWizard.passwd)
        $('passwd').value = ConfigRIPWizard.passwd;

      if ($(ConfigRIPWizard.failureId))
        $(ConfigRIPWizard.failureId).className = 'error';

      $('ifname').focus();
    };
    new Ajax.Updater({success: 'customContent'}, url,
                     {evalScripts: true, onComplete: updatePage});
  },

  gotoStep2: function(forward) {
    try {
      if (forward) {
        ConfigRIPWizard.ifname = $('ifname').value;
        ConfigRIPWizard.passwd = $('passwd').value;
        ConfigRIPWizard.checkNonempty(ConfigRIPWizard.ifname, 'ifname');
        ConfigRIPWizard.checkNonempty(ConfigRIPWizard.passwd, 'passwd');
      }

      var url = ConfD.baseUrl+'/configRIPWizard2.html';
      var updatePage = function() {

        if (ConfigRIPWizard.netmask)
          $('netmask').value = ConfigRIPWizard.netmask;

        if (ConfigRIPWizard.preflen)
          $('preflen').value = ConfigRIPWizard.preflen;

        if ($(ConfigRIPWizard.failureId))
          $(ConfigRIPWizard.failureId).className = 'error';

        $('netmask').focus();
      };
      new Ajax.Updater({success: 'customContent'}, url,
      {evalScripts: true, onComplete: updatePage});
    } catch (id) {
      ConfigRIPWizard.failureId = id;
      $(id).className = 'error';
    }
  },

  gotoStep3: function(forward) {
    try {
      if (forward) {

        ConfigRIPWizard.netmask = $('netmask').value;
        ConfigRIPWizard.preflen = $('preflen').value;
        ConfigRIPWizard.checkIp(ConfigRIPWizard.netmask, 'netmask');
        ConfigRIPWizard.checkPref(ConfigRIPWizard.preflen);
      }

      var url = ConfD.baseUrl+'/configRIPWizard3.html';
      var updatePage = function() {
        $('netmask').innerHTML = ConfigRIPWizard.netmask;
        $('preflen').innerHTML = ConfigRIPWizard.preflen;
        $('ifname1').innerHTML = $('ifname2').innerHTML = ConfigRIPWizard.ifname;
        $('passwd').innerHTML = ConfigRIPWizard.passwd;

        if ($(ConfigRIPWizard.failureId))
          $(ConfigRIPWizard.failureId).className = 'error';
      };
      new Ajax.Updater({success: 'customContent'}, url,
                       {evalScripts: true, onComplete: updatePage});
    } catch (id) {
      ConfigRIPWizard.failureId = id;
      $(id).className = 'error';
    }
  },

  save: function(proceed) {
    if (!proceed)
      proceed = function() {
        Navigator.redrawKeypath = '/system/router/rip';
        Tabs.setEditTab();
        Toolbar.changes();
      };

    var elems = [
        {id: '/system/key-chain', keys: ['ripauth'], value: [
            {id: 'key', keys: ['11'], value: [
                {id: 'key-string', value: ConfigRIPWizard.passwd}]}]},
        {id: '/system/interface', keys: [ConfigRIPWizard.ifname], value: [
            {id: 'ip/rip', value: [
                {id: 'authentication-mode', value: [
                    {id: 'mode', value: 'md5'},
                    {id: 'auth-length', value: 'rfc'}]},
                {id: 'authentication-key-chain', value: 'ripauth'}]}]},
        {id: '/system/router/rip', value: [
            {id: 'version', value: '2'},
            {id: 'network-ip', keys: [ConfigRIPWizard.netmask, ConfigRIPWizard.preflen]},
            {id: 'passive-interfaces', value: [
                {id: 'passive-by-default', value: 'true'},
                {id: 'except-interface', keys: [ConfigRIPWizard.ifname]}]},
            {id: 'route', keys: ['128.0.0.0', '1']}]}];
    ConfigRIPWizard.saveElems(elems, "", proceed);
  },

  saveElems: function(elems, root, proceed) {
    if (elems.length == 0)
      return proceed();
    var elem = elems.shift();
    var path = root + elem.id;
    if ('keys' in elem)
      path += '{' + elem.keys.join(' ') + '}';
    var process = function() {
      if (elem.value != null) {
        var cont = function() { ConfigRIPWizard.saveElems(elems, root, proceed); };
        if (typeof elem.value == 'string')
          ConfigRIPWizard.setElem(path, elem.value, cont);
        else
          ConfigRIPWizard.saveElems(elem.value, path + '/', cont);
      } else
        ConfigRIPWizard.saveElems(elems, root, proceed);
    };
    Maapi.exists(TabsOps.getTh(), path, function(res) {
        if (Maapi.isErrorResult(res))
          ConfigRIPWizard.handleErr(res);
        else
          if (res || (elem.value != null && typeof elem.value == 'string'))
            process();
          else
            Maapi.create(TabsOps.getTh(), path, function(res) {
                if (Maapi.isErrorResult(res)) {
                  // FIXME: The logic is broken here, i.e.
                  // /system/interface{eeth0}/ip/rip can not be created!
                  // I skip to report an error and call process() instead.
                  //ConfigRIPWizard.handleErr(res);
                  process();
                } else
                  process();
              });
      });
  },

  setElem: function(keypath, value, proceed) {
    Maapi.setElem(TabsOps.getTh(), keypath, value,
                  function(res) {
                    if (Maapi.isErrorResult(res)) {
                      Maapi.remove(TabsOps.getTh(), keypath);
                      ConfigRIPWizard.handleErr(res);
                    } else
                      proceed();
                  });
  },

  saveAndCommit: function() {
    var commit =
      function() {
        Maapi.validateCommit(TabsOps.getTh(), function(res) {
          if (Maapi.isErrorResult(res)) {
            ConfigRIPWizard.handleErr(res);
          } else
            Maapi.commit(TabsOps.getTh(), function(res) {
              if (Maapi.isErrorResult(res)) {
                ConfigRIPWizard.handleErr(res);
              } else {
                Maapi.newWriteTrans(TabsOps.mode, TabsOps.dbType,
                  function(th) {
                    if (Maapi.isErrorResult(th)) {
                      alert(th.error);
                      return;
                    } else {
                      TabsOps.writeTh = th;
                      Navigator.redrawKeypath = "/system/router/rip";
                      Tabs.setEditTab();
                    }
                  });
              }
            });
        });
      };
    ConfigRIPWizard.save(commit);
  },

  handleErr: function(res) {
    $('failureReason').innerHTML = res.error;
    $('failure').style.display = 'block';

    if (ConfigRIPWizard.failureId)
      $(ConfigRIPWizard.failureId).className = null;

    ConfigRIPWizard.failureId = 'ip';
    $('ip').className = 'error';
    $('port').className = 'error';
  },

  checkIp: function(value, id) {
    var re = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;

    if (!re.test(value))
      throw id;

    if (ConfigRIPWizard.failureId == id)
      ConfigRIPWizard.failureId = null;
  },

  checkPref: function(pref) {
    var re = /^\d{1,2}$/;
    if (!re.test(pref) || +pref <= 1 || +pref > 32)
      throw 'preflen';

    if (ConfigRIPWizard.failureId == 'preflen')
      ConfigRIPWizard.failureId = null;
  },

  checkNonempty: function(value, id) {
    if (value == null || value == "")
      throw id;

    if (ConfigRIPWizard.failureId == id)
      ConfigRIPWizard.failureId = null;
  },

  checkName: function(value) {
    if (value == "")
      throw 'name';

    if (ConfigRIPWizard.failureId == 'name')
      ConfigRIPWizard.failureId = null;
  }
}
