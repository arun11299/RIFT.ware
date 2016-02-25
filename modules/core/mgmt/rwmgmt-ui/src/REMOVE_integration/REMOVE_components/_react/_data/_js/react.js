// tutorial1.js
var converter = new Showdown.converter();

var data = [
  {author: "Pete Hunt", text: "This is one comment"},
  {author: "Jordan Walke", text: "This is *another* comment"}
];

var Comment = React.createClass({displayName: 'Comment',
  render: function() {
    var rawMarkup = converter.makeHtml(this.props.children.toString());
    return (
      React.createElement("div", {className: "comment"},
        React.createElement("h2", {className: "commentAuthor"},
          this.props.author
        ),
        React.createElement("span", {dangerouslySetInnerHTML: {__html: rawMarkup}})
      )
    );
  }
});

var CommentList = React.createClass({displayName: 'CommentList',
  render: function() {
    var commentNodes = this.props.data.map(function (comment) {
      return (
        React.createElement(Comment, {author: comment.author},
          comment.text
        )
      );
    });
    return (
      React.createElement("div", {className: "commentList"},
        commentNodes
      )
    );
  }
});

var CommentForm = React.createClass({displayName: 'CommentForm',
  render: function() {
    return (
      React.createElement("div", {className: "commentForm"},
        "Hello, world! I am a CommentForm."
      )
    );
  }
});

var CommentBox = React.createClass({displayName: 'CommentBox',
  getInitialState: function() {
    return {data:[
      {author: "Allan Dorken", text: "This is one comment"},
      {author: "Talvo Ran", text: "This is *another* comment"}
    ]};
  },
  componentDidMount: function() {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      success: function(data) {
        console.log(data);
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
        this.setState({data:this.data});
      }.bind(this)
    });
  },
  render: function() {
    return (
      React.createElement("div", {className: "commentBox"},
        React.createElement("h1", null, "Comments"),
        React.createElement(CommentList, {data: this.state.data}),
        React.createElement(CommentForm, null)
      )
    );
  }
});


React.render(
  React.createElement(CommentBox, {url: "comments.json"}),
  document.getElementById('content')
);

