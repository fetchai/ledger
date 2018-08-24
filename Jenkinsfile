pipeline {

  agent {
    docker {
      image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
    }
  }

  stages {
    stage ('Documentation') {

      when {
        expression {
          GIT_BRANCH = 'origin/' + sh(returnStdout: true, script: 'git rev-parse --abbrev-ref HEAD').trim()
          return GIT_BRANCH == 'origin/master' || params.FORCE_FULL_BUILD
        } // expression
      } // when

      steps {
        sh 'env'
      } // steps

    } // stage
  } // stages
} // pipeline

