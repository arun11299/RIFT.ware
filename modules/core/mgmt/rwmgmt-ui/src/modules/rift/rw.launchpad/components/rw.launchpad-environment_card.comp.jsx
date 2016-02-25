/**
 *
 * @jsx React.DOM
 */
var EnvironmentCard = React.createClass({
    displayName: 'EnvironmentCard',
    onClicked: function(action, syntheticEvent, reactId) {
        this.props.dispatcher.emit(action, this.props.data);
    },
    render: function() {
        var self = this;

        var elements = this.props.data.map(function(datum) {

            var status,
                uptime,
                uiTools,
                restart,
                stop,
                start,
                consoleClick,
                cliClick;


            if (datum.console) {
                consoleClick = <li><a onClick={self.onClicked.bind(null, 'console')}>Console</a></li>;
            } else {
                consoleClick = <li><a class="disabled">Console</a></li>;
            }

            if (datum.cli) {
                cliClick = <li><a class="disabled">CLI</a></li>;
            }

            if (datum.status == 'active') {
                status = <span class="icn c-grn fa-circle">{datum.status}</span>;
                uptime = <div run="true" initialtime="datum.uptime"></div>;
                restart = <li><a onClick={self.onClicked.bind(null, 'restart')}>Restart</a></li>;
                stop = <li><a onClick={self.onClicked.bind(null, 'stop')}>Stop</a></li>;
                uiTools = <ul>
                            <li><a onClick={self.onClicked.bind(null, 'webui')}>Web UI</a></li>
                            {consoleClick}
                            {cliClick}
                        </ul>;
            } else if (datum.status == 'Failed') {
                status = <span class="icn c-rd">{datum.status}</span>;
            } else if (datum.status == 'inactive') {
                status = <span class="icn fa-minus-circle">{datum.status}</span>;
                uptime = <div run="false" initialtime="inactive"></div>;
                start = <li><a onClick={self.onClicked.bind(null, 'start')}>Start</a></li>;
            }


            var template = (<li data-id="{datum.id}">
                        <dl>
                            <dt title="{datum.name}">{datum.name}</dt>
                            <dd class="templateType">{datum.template_name}</dd>
                            <dd class="infoBox">
                                <dl>
                                    <dt class="status">Status</dt>
                                    <dd class="status">
                                        {status}
                                    </dd>
                                    <dt class="uptime">Uptime</dt>
                                    <dd class="uptime" >
                                        {uptime}
                                    </dd>
                                    <dd class="description">
                                        <p>{datum.description}</p>
                                    </dd>
                                </dl>
                            </dd>
                            <dd class="uiTools">
                                {uiTools}
                            </dd>
                            <dd class="actions">
                                <ul>
                                    <li dropdown class="drpdwn-grp">
                                        <a class="drpdwn-tgl" dropdown-toggle>Actions <span class="icn fa-sort-desc"></span></a>
                                        <ul class="drpdwn" dropdown-menu role="menu">
                                            {restart}
                                            {stop}
                                            {start}
                                            <li><a class="disabled">Edit</a></li>
                                            <li><a onClick={self.onClicked.bind(null, 'delete')}>Delete</a></li>
                                            <li class="dvdr"></li>
                                            <li><a>Add to Launch Group</a></li>
                                            <li class="dvdr"></li>
                                            <li><a>Duplicate</a></li>
                                            <li><a>Snapshot</a></li>
                                        </ul>
                                    </li>
                                </ul>
                            </dd>
                            <dd class="clnup"></dd>
                        </dl>
                      </li>
                    );

            return template;
        });

        return <div>{elements}</div>;
    }
});