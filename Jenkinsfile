#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'bullseye',
        'centos6',
        'centos7',
        'centos8',
        'bionic',
        'stretch',
        'buster',
        'focal'
    ],
    target_repos: [
        'wallarm-node': [
            release: '3.4',
            dev: true
        ]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)
