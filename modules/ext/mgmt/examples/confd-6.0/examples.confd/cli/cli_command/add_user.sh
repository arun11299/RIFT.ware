#!/bin/bash

## Ask for user name
while true; do
    echo -n "Enter user name: "
    read user

    if [ ! -n "${user}" ]; then
	echo "You failed to supply a user name."
    elif maapi --exists "/aaa:aaa/authentication/users/user{${user}}"; then
	echo "The user already exists."
    else
	break
    fi
done

## Ask for password
while true; do
    echo -n "Enter password: "
    read -s pass1
    echo

    if [ "${pass1:0:1}" == "$" ]; then
	echo -n "The password must not start with $. Please choose a "
	echo    "different password."
    else
	echo -n "Confirm password: "
	read -s pass2
	echo

	if [ "${pass1}" != "${pass2}" ]; then
	    echo "Passwords do not match."
	else
	    break
	fi
    fi
done

groups=`maapi --keys "/aaa:aaa/authentication/groups/group"`
while true; do
    echo "Choose a group for the user."
    echo -n "Available groups are: "
    for i in ${groups}; do echo -n "${i} "; done    
    echo
    echo -n "Enter group for user: "
    read group

    if [ ! -n "${group}" ]; then
	echo "You must enter a valid group."
    else
	for i in ${groups}; do
	    if [ "${i}" == "${group}" ]; then
		# valid group found
		break 2;
	    fi
	done
	echo "You entered an invalid group."
    fi
    echo
done

echo "Creating user"

maapi --create "/aaa:aaa/authentication/users/user{${user}}"
maapi --set "/aaa:aaa/authentication/users/user{${user}}/password" "${pass1}"

echo "Setting home directory to: /var/confd/homes/${user}"
maapi --set "/aaa:aaa/authentication/users/user{${user}}/homedir" \
            "/var/confd/homes/${user}"

echo "Setting ssh key directory to: /var/confd/homes/${user}/ssh_keydir"
maapi --set "/aaa:aaa/authentication/users/user{${user}}/ssh_keydir" \
            "/var/confd/homes/${user}/ssh_keydir"

maapi --set "/aaa:aaa/authentication/users/user{${user}}/uid" "1000"
maapi --set "/aaa:aaa/authentication/users/user{${user}}/gid" "100"

echo "Adding user to the ${group} group."
gusers=`maapi --get "/aaa:aaa/authentication/groups/group{${group}}/users"`

for i in ${gusers}; do
    if [ "${i}" == "${user}" ]; then
	echo "User already in group"
	exit 0
    fi
done

maapi --set "/aaa:aaa/authentication/groups/group{${group}}/users" \
            "${gusers} ${user}"



