#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'centos7',
        'centos8',
        'xenial',
        'bionic',
        'stretch',
        'buster',
        'focal'
    ],
    target_repos: [
        'wallarm-node': [
		release: '2.19',
		dev: true
	]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)
