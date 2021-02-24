server_sources = ['user/blockedusers.c', 'user/blockedusers.h', 'clients.c', 'clients.h', 'commands.c', 'commands.h', 
    'user/message.c', 'user/message.h', 'server.c', 'server.h', 'serverClients.c', 'serverClients.h', 'user/user.c', 'user/user.h']
executable('server', server_sources,
            link_with: common_lib_static,
            install: true,
            include_directories: '..',
            dependencies: [threads])

# dependencies: [openssl],
