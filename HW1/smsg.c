    char msg[][100] = {
        "[Error] Operation failed. Please try again.\n",
        "Locked.\n",
        "Please enter your id (to check your preference order):\n",
        "Your preference order is %s > %s > %s.\n",
        "Please input your preference order respectively(AZ,BNT,Moderna):",
        "Preference order for %d modified successed, new preference order is %s > %s > %s.\n"
    };

    int msglen[] = {
        strlen(msg[0]),
        strlen(msg[1]),
        strlen(msg[2]),
        strlen(msg[3]),
    };