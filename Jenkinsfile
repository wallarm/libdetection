#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'centos7',
        'centos8',
        'bionic',
        'stretch',
        'buster',
        'focal'
    ],
    target_repos: [
        'wallarm-node': [
		release: '3.1',
		dev: true
	]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)
